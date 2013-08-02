#include <iostream>
#include <fstream>
#include <cstring>
#include <cerrno>

#include "util/tokenizer.h"
#include "util/exception.h"
#include "util/filesystem.h"
#include "util/string.h"
#include "mve/image.h"
#include "mve/view.h"

/* The signature to identify MVE files. */
#define MVE_FILE_SIGNATURE "\211MVE\n"
#define MVE_FILE_SIGNATURE_LEN 5

MVE_NAMESPACE_BEGIN

bool
MVEFileProxy::check_direct_write (void) const
{
    /* Image needs to be cached. */
    if (this->image == NULL)
        return false;

    /* Proxy needs to be bound to a file. */
    if (this->file_pos == 0 || this->byte_size == 0)
        return false;

    /* Image (or data) dimensions must be the same. */
    if (this->width != this->image->width()
        || this->height != this->image->height()
        || this->channels != this->image->channels())
        return false;

    /* Image (or data) datatype must be the same. */
    if (this->datatype != this->image->get_type_string())
        return false;

    return true;
}

/* ---------------------------------------------------------------- */

ImageType
MVEFileProxy::get_type (void) const
{
    if (this->image != NULL)
        return this->image->get_type();
    return ImageBase::get_type_for_string(this->datatype);
}

/* ---------------------------------------------------------------- */

bool
MVEFileProxy::is_type (std::string const& typestr) const
{
    return (image != NULL && image->get_type_string() == typestr)
        || this->datatype == typestr;
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void
View::load_mve_file (std::string const& filename, bool merge)
{
    if (filename.empty())
        throw std::invalid_argument("No filename given");
    this->filename = filename;

    /* Check if write locks are set. Wait for release. */
    util::fs::FileLock lock;
    if (!lock.wait_lock(this->filename))
        throw util::Exception("File is locked: ", lock.get_reason());

    /* Open file. */
    std::ifstream infile(filename.c_str(), std::ios::binary);
    if (!infile.good())
        throw util::FileException(filename, std::strerror(errno));

    /* Signature checks. */
    {
        /* Read file signature. */
        char buf[MVE_FILE_SIGNATURE_LEN];
        infile.read(buf, MVE_FILE_SIGNATURE_LEN);
        if (infile.eof())
        {
            infile.close();
            throw util::Exception("Premature end of file");
        }

        /* Check file signature. */
        bool valid = true;
        for (std::size_t i = 0; i < MVE_FILE_SIGNATURE_LEN; ++i)
            if (buf[i] != MVE_FILE_SIGNATURE[i])
                valid = false;

        if (!valid)
        {
            infile.close();
            throw util::Exception("Invalid file signature");
        }
    }

    /* Remember old proxies and start with empty list. */
    Proxies old_proxies;
    MVEFileMeta old_meta;
    CameraInfo old_camera;
    std::swap(this->proxies, old_proxies);
    std::swap(this->meta, old_meta);
    std::swap(this->camera, old_camera);

    /* Read headers. */
    while (true)
    {
        std::string buf;
        std::getline(infile, buf);
        if (buf == "end_headers")
            break;

        try
        {
            if (infile.eof())
                throw util::Exception("Premature end of file during headers");

            this->parse_header_line(buf);

            if (this->proxies.size() > 128)
                throw util::Exception("Spurious amount of embeddings!");
        }
        catch (util::Exception& e)
        {
            infile.close();
            std::swap(this->proxies, old_proxies);
            std::swap(this->meta, old_meta);
            std::swap(this->camera, old_camera);
            throw e;
        }
    }

    /* Done reading input file. */
    std::size_t current_pos = infile.tellg();
    infile.close();

    /* Update the camera information. */
    this->update_camera();

    /* Compute file_pos and byte_size for all embeddings. */
    for (std::size_t i = 0; i < this->proxies.size(); ++i)
    {
        MVEFileProxy& p(this->proxies[i]);

        /*
         * Compute file_pos and advance current pos. The intro in front
         * of each embedding contains the letters "embedding", the name
         * and byte size of the embedding, two blanks and a newline.
         */
        std::size_t intro_len = 12 + p.name.size()
            + util::string::get(p.byte_size).size();
        p.file_pos = current_pos + intro_len;
        current_pos = p.file_pos + p.byte_size + 1; // +1 for trailing newline

        //std::cout << "Guessed parameters for " << p.name << ": size "
        //    << p.byte_size << ", pos " << p.file_pos << std::endl;
    }

    if (!merge)
        return;

    /*
     * Merge the old represenation with the new one. The merge should
     *
     * - keep old meta info (only if needs_rebuild is set?),
     * - keep old camera info (only if needs_rebuild is set?),
     * - keep old embeddings that are dirty,
     * - keep old embeddings that are in use (cached and references around).
     *
     * Direct writing must be disabled (i.e. force_rewrite) if the meta
     * information changed, the amount of embeddings changed,
     * or size of cached embeddings changed.
     *
     * Problem case: Embedding content changed in background, embedding is
     *   currently cached but not dirty. Detect & write back? Ignore?
     *   -> Easy solution: ignore
     *
     * The flag 'needs_rebuild' becomes true if
     *
     * - view ID or view name changes,
     * - embeddings are removed or added,
     * - type of embedding (data/image) is changed,
     * - the camera is set.
     *
     * Current issues: Althogh merging may work, the issue is that merging
     * creates completely new proxies. This is problematic as pointers
     * to proxies may be around (passed to ensure_embedding). Also, images
     * that are cached but not used are uncached. This may impact speed.
     */
    std::cout << "Merging file with memory..." << std::endl;
    this->meta = old_meta;
    this->camera = old_camera;
    for (std::size_t i = 0; i < old_proxies.size(); ++i)
    {
        MVEFileProxy const& p = old_proxies[i];
        if (p.is_dirty || p.image.use_count() > 1)
        {
            MVEFileProxy* p2 = this->get_proxy_intern(p.name);
            if (p2)
            {
                std::cout << "  Reverting embedding" << std::endl;
                *p2 = p;
            }
            else
            {
                std::cout << "  Keeping embedding" << std::endl;
                this->proxies.push_back(p);
            }
        }
    }
}

/* ---------------------------------------------------------------- */

void
View::parse_header_line (std::string const& header_line)
{
    /* Clean header line. */
    std::string str(header_line);
    util::string::clip_newlines(&str);
    util::string::clip_whitespaces(&str);
    util::string::normalize(&str);

    /* Tokenize header. */
    util::Tokenizer tokens;
    tokens.split(str);

    if (tokens.empty())
        throw util::Exception("Error: Invalid header line");

    if (tokens[0] == "image")
    {
        if (tokens.size() != 6)
            throw util::Exception("Invalid image header: ", str);

        MVEFileProxy p;
        p.is_image = true;
        p.name = tokens[1];
        p.width = util::string::convert<int>(tokens[2]);
        p.height = util::string::convert<int>(tokens[3]);
        p.channels = util::string::convert<int>(tokens[4]);
        p.datatype = tokens[5];
        int type_size = util::string::size_for_type_string(p.datatype);
        if (!type_size)
            throw util::Exception("Invalid image type: ", p.datatype);
        p.byte_size = p.width * p.height * p.channels * type_size;
        this->proxies.push_back(p);
    }
    else if (tokens[0] == "data")
    {
        if (tokens.size() != 3)
            throw util::Exception("Invalid data header: ", str);

        MVEFileProxy p;
        p.is_image = false;
        p.name = tokens[1];
        p.width = util::string::convert<int>(tokens[2]);
        p.height = 1;
        p.channels = 1;
        p.datatype = "uint8";
        p.byte_size = p.width;
        this->proxies.push_back(p);
    }
    else if (tokens[0] == "id")
    {
        if (tokens.size() != 2)
            throw util::Exception("Invalid ID header: ", str);
        this->meta.view_id = util::string::convert<std::size_t>(tokens[1]);
    }
    else if (tokens[0] == "name")
    {
        if (tokens.size() <= 1)
            throw util::Exception("Invalid view name header: ", str);
        this->meta.view_name = tokens.concat(1);
    }
    else if (tokens[0] == "camera") // + 15 tokens (t,R,fl,rd1,rd2)
    {
        // This is DEPRECATED. Remove it at some time...
        if (tokens.size() != 16)
            throw util::Exception("Invalid camera spec: ", str);
        this->meta.camera_ext_str = tokens.concat(1, 12);
        this->meta.camera_int_str = tokens.concat(13);
    }
    else if (tokens[0] == "camera-ext") // + 12 tokens (t,R)
    {
        if (tokens.size() != 13)
            throw util::Exception("Invalid camera spec: ", str);
        this->meta.camera_ext_str = tokens.concat(1, 12);
    }
    else if (tokens[0] == "camera-int") // + 1 to 7 tokens
    {
        if (tokens.size() < 2 || tokens.size() > 8)
            throw util::Exception("Invalid camera spec: ", str);
        this->meta.camera_int_str = tokens.concat(1);
    }
    else
        throw util::Exception("Unrecognized header: ", tokens[0]);
}

/* ---------------------------------------------------------------- */

void
View::update_camera (void)
{
    if (!this->meta.camera_ext_str.empty())
        this->camera.from_ext_string(this->meta.camera_ext_str);
    if (!this->meta.camera_int_str.empty())
        this->camera.from_int_string(this->meta.camera_int_str);
}

/* ---------------------------------------------------------------- */

void
View::save_mve_file_as (std::string const& filename)
{
    if (filename.empty())
        throw std::invalid_argument("No filename given");

    // TODO: Re-read and merge with file on disc?
    //this->reload_mve_file(true);

    std::string file_component = util::fs::get_file_component(filename);
    std::cout << "Saving MVE file as '" << file_component << "'..." << std::endl;

    /* Acquire file lock for the view. */
    util::fs::FileLock lock;
    util::fs::FileLock::Status lock_status = lock.acquire_retry(filename);
    if (lock_status != util::fs::FileLock::LOCK_CREATED)
        throw util::Exception("Cannot acquire lock: ", lock.get_reason());

    /*
     * Cache all embeddings before opening output file. This is important
     * to prevent troubles when the new, given filename refers to the same
     * file than the source file, from which embeddings need to be read.
     */
    for (std::size_t i = 0; i < this->proxies.size(); ++i)
    {
        MVEFileProxy& p(this->proxies[i]);
        this->ensure_embedding(p); // TODO: Use function that does not merge
        p.width = p.image->width();
        p.height = p.image->height();
        p.channels = p.image->channels();
        p.byte_size = p.image->get_byte_size();
        p.datatype = p.image->get_type_string();
    }

    /* Open output file. */
    std::ofstream out(filename.c_str());
    if (!out.good())
        throw util::Exception("Cannot open file: ", filename);

    /* Write file signature. */
    out.write(MVE_FILE_SIGNATURE, MVE_FILE_SIGNATURE_LEN);

    /* Write meta headers. */
    std::stringstream header_ss;
    if (meta.view_id != (std::size_t)-1)
        header_ss << "id " << this->meta.view_id << "\n";
    if (!meta.view_name.empty())
        header_ss << "name " << this->meta.view_name << "\n";
    if (!meta.camera_ext_str.empty())
        header_ss << "camera-ext " << this->meta.camera_ext_str << "\n";
    if (!meta.camera_int_str.empty())
        header_ss << "camera-int " << this->meta.camera_int_str << "\n";

    std::string header_str(header_ss.str());
    if (!header_str.empty())
        out.write(header_str.c_str(), header_str.size());

    /* Write embedding headers. */
    for (std::size_t i = 0; i < this->proxies.size(); ++i)
    {
        /* Update proxy information. */
        MVEFileProxy const& p(this->proxies[i]);
        if (p.is_image)
        {
            out << "image " << p.name << " " << p.width << " " << p.height
                << " " << p.channels << " " << p.datatype << "\n";
        }
        else
        {
            out << "data " << p.name << " " << p.byte_size << "\n";
        }
    }

    /* Finalize headers. */
    out << "end_headers" << "\n";

    /* Write images and data. */
    for (std::size_t i = 0; i < this->proxies.size(); ++i)
    {
        /* Write embedding (and update file_pos proxy information). */
        MVEFileProxy& p(this->proxies[i]);
        out << "embedding " << p.name << " " << p.byte_size << "\n";
        p.file_pos = out.tellp();
        p.is_dirty = false;
        out.write(p.image->get_byte_pointer(), p.byte_size);
        out.write("\n", 1);
    }

    /* Finalize file and update members. */
    out.write("EOF\n", 4);
    out.close();
    this->filename = filename;
    this->needs_rebuild = false;

    /* Because all embeddings are now cached, we release some memory. */
    this->cache_cleanup();

    std::cout << "Done saving file as '" << file_component << "'." << std::endl;
}

/* ---------------------------------------------------------------- */

void
View::save_mve_file (bool force_rebuild)
{
    if (this->filename.empty())
        throw std::invalid_argument("No filename set");

    // TODO: Re-read and merge with file on disc?
    //this->reload_mve_file(true);

    /* For debugging... */
    std::string file_component = util::fs::get_file_component(this->filename);

    /*
     * Check if we can write embeddings directly to file instead of creating
     * a new file from scratch. Only dirty embeddings are of interest.
     */
    bool direct = false;
    if (!force_rebuild && !this->needs_rebuild)
    {
        direct = true;
        std::size_t num_dirty = 0;
        for (std::size_t i = 0; i < this->proxies.size(); ++i)
        {
            if (!this->proxies[i].is_dirty)
                continue;

            num_dirty += 1;
            if (!this->proxies[i].check_direct_write())
                direct = false;
        }

        if (num_dirty == 0)
        {
            std::cout << "Nothing changed for '" << file_component
                << "', skipping." << std::endl;
            return;
        }
    }

    /* Acquire file lock for the view. */
    util::fs::FileLock lock;
    util::fs::FileLock::Status lock_status = lock.acquire_retry(this->filename);
    if (lock_status != util::fs::FileLock::LOCK_CREATED)
        throw util::Exception("Cannot acquire lock: ", lock.get_reason());

    /* Write embeddings directly to view file. */
    bool success = false;
    if (direct)
    {
        std::cout << "Direct-writing modified data to "
            << file_component << std::endl;
        try
        {
            for (std::size_t i = 0; i < this->proxies.size(); ++i)
                if (this->proxies[i].is_dirty)
                    this->direct_write(this->proxies[i]);
            success = true;
        }
        catch (util::Exception& e)
        {
            std::cout << "Error direct-writing to " << file_component
                << ": " << e << std::endl;
        }
    }

    /* Store the view by rebuilding the file from scratch. */
    if (!success)
    {
        std::string orig_filename = this->filename;
        this->save_mve_file_as(this->filename + ".new");
        util::fs::unlink(orig_filename.c_str());
        this->rename_file(orig_filename);
    }

    std::cout << "Done saving '" << file_component << "'." << std::endl;
}

/* ---------------------------------------------------------------- */

void
View::direct_write (MVEFileProxy& p)
{
    if (this->filename.empty())
        throw std::invalid_argument("No filename given");

    if (p.byte_size == 0 || p.file_pos == 0)
        throw util::Exception("Proxy not properly initialized");

    if (p.image == NULL || p.image->get_byte_size() != p.byte_size)
        throw util::Exception("Cannot directly write to view");

    std::fstream out(this->filename.c_str(),
        std::ios::in | std::ios::out | std::ios::binary);
    if (!out.good())
        throw util::Exception("Error opening MVE file: ",
            std::strerror(errno));

    out.seekp(p.file_pos);
    if (!out.good())
    {
        out.close();
        throw util::Exception("Error seeking in MVE file: ",
            std::strerror(errno));
    }
    out.write(p.image->get_byte_pointer(), p.image->get_byte_size());
    out.close();

    if (out.bad())
        throw util::Exception("Error writing to MVE file: ",
            std::strerror(errno));

    p.is_dirty = false;
}

/* ---------------------------------------------------------------- */

bool
View::rename_file (std::string const& new_name)

{
    bool good = util::fs::rename(this->filename.c_str(), new_name.c_str());
    this->filename = new_name;
    return good;
}

/* ---------------------------------------------------------------- */

void
View::load_embedding (MVEFileProxy& p)
{
    if (this->filename.empty() || p.byte_size == 0 || p.file_pos == 0)
        throw util::Exception("Proxy not properly initialized");

    if (p.is_image && (!p.width || !p.height || !p.channels))
        throw util::Exception("Image with invalid image dimensions");

    /* Open MVE input file. */
    std::ifstream mvefile(this->filename.c_str(), std::ios::binary);
    if (!mvefile.good())
        throw util::FileException(filename, std::strerror(errno));

    /* Allocate memory for image or data embedding. */
    if (p.is_image)
    {
        if (p.datatype == "uint8")
            p.image = ByteImage::create(p.width, p.height, p.channels);
        else if (p.datatype == "uint16")
            p.image = RawImage::create(p.width, p.height, p.channels);
        else if (p.datatype == "float")
            p.image = FloatImage::create(p.width, p.height, p.channels);
        else if (p.datatype == "double")
            p.image = DoubleImage::create(p.width, p.height, p.channels);
        else if (p.datatype == "sint32")
            p.image = IntImage::create(p.width, p.height, p.channels);
        else
        {
            mvefile.close();
            throw util::Exception("Unrecognized image data type");
        }
    }
    else
    {
        ByteImage::Ptr img(ByteImage::create());
        img->allocate(p.byte_size, 1, 1);
        p.image = img;
    }

    /* Seek and read embedding. */
    mvefile.seekg(p.file_pos);
    mvefile.read(p.image->get_byte_pointer(), p.image->get_byte_size());
    if (mvefile.eof())
    {
        mvefile.close();
        throw util::Exception("Unexpected EOF");
    }
    //mvefile.get(); // Discard final newline
    mvefile.close();
}

/* ---------------------------------------------------------------- */

void
View::ensure_embedding (MVEFileProxy& proxy)
{
    //if (sync_file)
    //    this->reload_mve_file(true);

    util::AtomicMutex<int> lock(this->mutex);
    if (proxy.image == NULL)
        this->load_embedding(proxy);
    lock.release();
}

/* ---------------------------------------------------------------- */

MVEFileProxy const*
View::get_proxy (std::string const& name) const
{
    for (std::size_t i = 0; i < this->proxies.size(); ++i)
        if (this->proxies[i].name == name)
            return &this->proxies[i];
    return NULL;
}

/* ---------------------------------------------------------------- */

MVEFileProxy*
View::get_proxy_intern (std::string const& name)
{
    for (std::size_t i = 0; i < this->proxies.size(); ++i)
        if (this->proxies[i].name == name)
            return &this->proxies[i];
    return NULL;
}

/* ---------------------------------------------------------------- */

ImageBase::Ptr
View::get_embedding (std::string const& name)
{
    MVEFileProxy* p(this->get_proxy_intern(name));
    if (p == NULL)
        return ImageBase::Ptr();
    this->ensure_embedding(*p);
    return p->image;
}

/* ---------------------------------------------------------------- */

bool
View::remove_embedding (std::string const& name)
{
    int num_erased = 0;
    for (Proxies::iterator iter = this->proxies.begin();
        iter != this->proxies.end();)
    {
        if (iter->name == name)
        {
            iter = this->proxies.erase(iter);
            num_erased += 1;
        }
        else
            iter++;
    }

    if (num_erased)
        this->needs_rebuild = true;

    return num_erased > 0;
}

/* ---------------------------------------------------------------- */

std::size_t
View::count_image_embeddings (void) const
{
    std::size_t amount = 0;
    for (std::size_t i = 0; i < this->proxies.size(); ++i)
        amount += this->proxies[i].is_image;
    return amount;
}

/* ---------------------------------------------------------------- */

void
View::set_image (std::string const& name, ImageBase::Ptr image)
{
    if (image == NULL)
        throw std::invalid_argument("NULL image passed");

    /* If there is no such proxy, add a new one. */
    MVEFileProxy* p = this->get_proxy_intern(name);
    if (p == NULL)
    {
        this->add_image(name, image);
        return;
    }

    /* A change from type 'data' to type 'image' requires a rebuild. */
    if (p->is_image == false)
        this->needs_rebuild = true;

    /* Update the current embedding by that name. */
    p->image = image;
    p->is_image = true;
    p->is_dirty = true;
}

/* ---------------------------------------------------------------- */

void
View::add_image (std::string const& name, ImageBase::Ptr image)
{
    if (image == NULL)
        throw std::invalid_argument("NULL image passed");

    if (this->has_embedding(name))
        throw util::Exception("Embedding already exists: ", name);

    MVEFileProxy p;
    p.name = name;
    p.image = image;
    p.is_image = true;
    p.is_dirty = true;
    this->proxies.push_back(p);
    this->needs_rebuild = true;
}

/* ---------------------------------------------------------------- */

ImageBase::Ptr
View::get_image (std::string const& name)
{
    MVEFileProxy* p(this->get_proxy_intern(name));
    if (p == NULL || !p->is_image)
        return ImageBase::Ptr();
    this->ensure_embedding(*p);
    return p->image;
}

/* ---------------------------------------------------------------- */

FloatImage::Ptr
View::get_float_image (std::string const& name)
{
    MVEFileProxy* p(this->get_proxy_intern(name));
    if (p == NULL || !p->is_image || !p->is_type(util::string::for_type<float>()))
        return FloatImage::Ptr();
    this->ensure_embedding(*p);
    return p->image;
}

/* ---------------------------------------------------------------- */

DoubleImage::Ptr
View::get_double_image (std::string const& name)
{
    MVEFileProxy* p(this->get_proxy_intern(name));
    if (p == NULL || !p->is_image || !p->is_type(util::string::for_type<double>()))
        return DoubleImage::Ptr();
    this->ensure_embedding(*p);
    return p->image;
}

/* ---------------------------------------------------------------- */

ByteImage::Ptr
View::get_byte_image (std::string const& name)
{
    MVEFileProxy* p(this->get_proxy_intern(name));
    if (p == NULL || !p->is_image || !p->is_type(util::string::for_type<uint8_t>()))
        return ByteImage::Ptr();
    this->ensure_embedding(*p);
    return p->image;
}

/* ---------------------------------------------------------------- */

IntImage::Ptr
View::get_int_image (std::string const& name)
{
    MVEFileProxy* p(this->get_proxy_intern(name));
    if (p == NULL || !p->is_image || !p->is_type(util::string::for_type<int>()))
        return IntImage::Ptr();
    this->ensure_embedding(*p);
    return p->image;
}

/* ---------------------------------------------------------------- */

void
View::set_data (std::string const& name, ByteImage::Ptr data)
{
    // Heads up: Almost duplicated code in this::set_image

    if (data == NULL)
        throw std::invalid_argument("NULL image passed");

    if (data->height() != 1 || data->channels() != 1)
        throw std::invalid_argument("Plain data with invalid dimensions");

    /* If there is no such proxy, add a new one. */
    MVEFileProxy* p = this->get_proxy_intern(name);
    if (p == NULL)
    {
        this->add_data(name, data);
        return;
    }

    /* A change from type 'data' to type 'image' requires a rebuild. */
    if (p->is_image == true)
        this->needs_rebuild = true;

    /* Update the current embedding by that name. */
    p->image = data;
    p->is_image = false;
    p->is_dirty = true;
}

/* ---------------------------------------------------------------- */

void
View::add_data (std::string const& name, ByteImage::Ptr data)
{
    // Heads up: Almost duplicated code in this::add_image

    if (data == NULL)
        throw std::invalid_argument("NULL image passed");

    if (data->height() != 1 || data->channels() != 1)
        throw std::invalid_argument("Plain data has more dimensions");

    if (this->has_embedding(name))
        throw util::Exception("Embedding already exists: ", name);

    MVEFileProxy p;
    p.name = name;
    p.image = data;
    p.is_image = false;
    p.is_dirty = true;
    this->proxies.push_back(p);
    this->needs_rebuild = true;
}

/* ---------------------------------------------------------------- */

ByteImage::Ptr
View::get_data (std::string const& name)
{
    // Heads up: Almost duplicated code in this::get_image

    MVEFileProxy* p(this->get_proxy_intern(name));
    if (p == NULL || p->is_image)
        return ByteImage::Ptr();
    this->ensure_embedding(*p);
    return p->image;
}

/* ---------------------------------------------------------------- */

bool
View::is_dirty (void) const
{
    if (this->needs_rebuild)
        return true;

    for (std::size_t i = 0; i < this->proxies.size(); ++i)
        if (this->proxies[i].is_dirty)
            return true;

    return false;
}

/* ---------------------------------------------------------------- */

std::size_t
View::cache_cleanup (void)
{
    std::size_t released = 0;
    for (std::size_t i = 0; i < this->proxies.size(); ++i)
    {
        MVEFileProxy& p(this->proxies[i]);
        if (p.is_dirty || p.image.use_count() != 1)
            continue;
        p.image.reset();
        released += 1;
    }
    return released;
}

/* ---------------------------------------------------------------- */

std::size_t
View::get_byte_size (void) const
{
    std::size_t ret = 0;
    for (std::size_t i = 0; i < this->proxies.size(); ++i)
    {
        MVEFileProxy const& p(this->proxies[i]);
        if (p.image != NULL)
            ret += p.image->get_byte_size();
    }
    return ret;
}

/* ---------------------------------------------------------------- */

void
View::set_camera (CameraInfo const& camera)
{
    std::string ext_str = camera.to_ext_string();
    std::string int_str = camera.to_int_string();
    if (this->meta.camera_ext_str != ext_str
        || this->meta.camera_int_str != int_str)
    {
        this->meta.camera_ext_str = ext_str;
        this->meta.camera_int_str = int_str;
        this->needs_rebuild = true;
    }
    this->camera = camera;
}

/* ---------------------------------------------------------------- */

void
View::print_debug (void) const
{
    std::cout << "---- View debug information ----" << std::endl;
    std::cout << "View name: " << this->meta.view_name << ", "
        << this->proxies.size() << " embeddings" << std::endl;

    for (std::size_t i = 0; i < this->proxies.size(); ++i)
    {
        MVEFileProxy const* p = &this->proxies[i];

        std::cout << "Proxy " << i << ": " << p->name << std::endl;
        std::cout << "  is image: " << p->is_image
            << ", is dirty: " << p->is_dirty
            << ", is cached: " << (p->image != NULL)
            << std::endl;;

        if (p->is_image)
            std::cout << "  image dimensions: " << p->width << "x"
                << p->height << "x" << p->channels
                << " (" << p->datatype << ")" << std::endl;
        else
            std::cout << "  data dimensions: " << p->width << std::endl;

        if (p->image != NULL)
        {
            ImageBase::Ptr img = p->image;
            std::cout << "  CACHED DATA: Type "
                << img->get_type_string() << ", "
                << img->width() << "x"
                << img->height() << "x"
                << img->channels()
                << (p->is_dirty ? " (IS DIRTY)" : "")
                << std::endl;
        }
    }
}

MVE_NAMESPACE_END
