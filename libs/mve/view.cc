#include <fstream>
#include <iostream>
#include <cstring>
#include <cerrno>

#include "util/exception.h"
#include "util/file_system.h"
#include "util/tokenizer.h"
#include "mve/view.h"
#include "mve/image_io.h"

#define VIEW_IO_META_FILE "meta.ini" // TODO
#define VIEW_IO_BLOB_SIGNATURE "\211MVE_BLOB\n"
#define VIEW_IO_BLOB_SIGNATURE_LEN 10

/* The signature to identify deprecated MVE files. */
#define VIEW_MVE_FILE_SIGNATURE "\211MVE\n"
#define VIEW_MVE_FILE_SIGNATURE_LEN 5

MVE_NAMESPACE_BEGIN

void
View::load_view (std::string const& path)
{
    std::cout << "View: Loading view: " << path << std::endl;

    /* Open meta.ini and populate images and blobs. */
    this->clear();
    try
    {
        this->load_meta_data(path);
        this->populate_images_and_blobs(path);
        this->path = path;
    }
    catch (...)
    {
        this->clear();
        throw;
    }
}

void
View::load_view_from_mve_file  (std::string const& filename)
{
    /* Open file. */
    std::ifstream infile(filename.c_str(), std::ios::binary);
    if (!infile.good())
        throw util::FileException(filename, std::strerror(errno));

    /* Signature checks. */
    char signature[VIEW_MVE_FILE_SIGNATURE_LEN];
    infile.read(signature, VIEW_MVE_FILE_SIGNATURE_LEN);
    if (!std::equal(signature, signature + VIEW_MVE_FILE_SIGNATURE_LEN,
        VIEW_MVE_FILE_SIGNATURE))
        throw util::Exception("Invalid file signature");

    this->clear();

    /* Read headers and create a schedule to read embeddings. */
    typedef std::pair<std::size_t, char*> ReadBuffer;
    std::vector<ReadBuffer> embedding_buffers;
    while (true)
    {
        std::string line;
        std::getline(infile, line);
        if (infile.eof())
            throw util::Exception("Premature EOF while reading headers");

        util::string::clip_newlines(&line);
        util::string::clip_whitespaces(&line);
        if (line == "end_headers")
            break;

        /* Tokenize header. */
        util::Tokenizer tokens;
        tokens.split(line);
        if (tokens.empty())
            throw util::Exception("Invalid header line: ", line);

        /* Populate meta data from headers. */
        if (tokens[0] == "image" && tokens.size() == 6)
        {
            ImageProxy proxy;
            proxy.is_dirty = true;
            proxy.name = tokens[1];
            proxy.width = util::string::convert<int>(tokens[2]);
            proxy.height = util::string::convert<int>(tokens[3]);
            proxy.channels = util::string::convert<int>(tokens[4]);
            proxy.type = mve::ImageBase::get_type_for_string(tokens[5]);
            proxy.is_initialized = true;
            proxy.image = mve::image::create_for_type
                (proxy.type, proxy.width, proxy.height, proxy.channels);
            this->images.push_back(proxy);
            embedding_buffers.push_back(ReadBuffer(
                proxy.image->get_byte_size(),
                proxy.image->get_byte_pointer()));
        }
        else if (tokens[0] == "data" && tokens.size() == 3)
        {
            BlobProxy proxy;
            proxy.is_dirty = true;
            proxy.name = tokens[1];
            proxy.size = util::string::convert<int>(tokens[2]);
            proxy.is_initialized = true;
            proxy.blob = mve::ByteImage::create(proxy.size, 1, 1);
            this->blobs.push_back(proxy);
            embedding_buffers.push_back(ReadBuffer(
                proxy.blob->get_byte_size(),
                proxy.blob->get_byte_pointer()));
        }
        else if (tokens[0] == "id" && tokens.size() == 2)
        {
            this->set_value("view.id", tokens[1]);
        }
        else if (tokens[0] == "name" && tokens.size() > 1)
        {
            this->set_value("view.name", tokens.concat(1));
        }
        else if (tokens[0] == "camera-ext" && tokens.size() == 13)
        {
            this->set_value("camera.translation", tokens.concat(1, 3));
            this->set_value("camera.rotation", tokens.concat(4, 9));
        }
        else if (tokens[0] == "camera-int" && tokens.size() > 2 && tokens.size() <= 8)
        {
            this->set_value("camera.focal_length", tokens[1]);
            if (tokens.size() > 4)
                this->set_value("camera.pixel_aspect", tokens[4]);
            if (tokens.size() > 6)
                this->set_value("camera.principal_point", tokens.concat(5, 2));
        }
        else
        {
            std::cerr << "Unrecognized header: " << line << std::endl;
        }
    }

    /* Read embeddings and populate view. */
    for (std::size_t i = 0; i < embedding_buffers.size(); ++i)
    {
        std::string line;
        std::getline(infile, line);
        if (infile.eof())
            throw util::Exception("Premature EOF while reading payload");

        util::Tokenizer tokens;
        tokens.split(line);
        if (tokens.size() != 3)
            throw util::Exception("Invalid embedding: ", line);

        ReadBuffer& buffer = embedding_buffers[i];
        std::size_t byte_size = util::string::convert<std::size_t>(tokens[2]);
        if (byte_size != buffer.first)
            throw util::Exception("Unexpected embedding size");

        infile.read(buffer.second, buffer.first);
        infile.ignore();
    }

    if (infile.eof())
        throw util::Exception("Premature EOF while reading payload");
    infile.close();
}

void
View::reload_view (void)
{
    if (this->path.empty())
        throw std::runtime_error("View not initialized");
    this->load_view(this->path);
}

void
View::save_view_as (std::string const& path)
{
    std::cout << "View: Saving view: " << path << std::endl;

    /* Create view directory if needed. */
    if (util::fs::file_exists(path.c_str()))
        throw util::FileException(path, "Is not a directory");
    if (!util::fs::dir_exists(path.c_str()))
        if (!util::fs::mkdir(path.c_str()))
            throw util::FileException(path, std::strerror(errno));

    /* Load all images and BLOBS. */
    for (std::size_t i = 0; i < this->images.size(); ++i)
        this->load_image(&this->images[i], false);
    for (std::size_t i = 0; i < this->blobs.size(); ++i)
        this->load_blob(&this->blobs[i], false);

    /* Save meta data, images and BLOBS, and free memory. */
    this->save_meta_data(path);
    this->path = path;
    this->save_view();
    this->cache_cleanup();
}

int
View::save_view (void)
{
    if (this->path.empty())
        throw std::runtime_error("View not initialized");

    int saved = 0;
    if (this->meta_data.is_dirty)
    {
        this->save_meta_data(this->path);
        saved += 1;
    }
    for (std::size_t i = 0; i < this->images.size(); ++i)
        if (this->images[i].is_dirty)
        {
            this->save_image_intern(&this->images[i]);
            saved += 1;
        }
    for (std::size_t i = 0; i < this->blobs.size(); ++i)
        if (this->blobs[i].is_dirty)
        {
            this->save_blob_intern(&this->blobs[i]);
            saved += 1;
        }
    return saved;
}

void
View::clear (void)
{
    this->meta_data.data.clear();
    this->images.clear();
    this->blobs.clear();
    this->path.clear();
}

bool
View::is_dirty (void) const
{
    if (this->meta_data.is_dirty)
        return true;
    for (std::size_t i = 0; i < this->images.size(); ++i)
        if (this->images[i].is_dirty)
            return true;
    for (std::size_t i = 0; i < this->blobs.size(); ++i)
        if (this->blobs[i].is_dirty)
            return true;
    return false;
}

int
View::cache_cleanup (void)
{
    int released = 0;
    for (std::size_t i = 0; i < this->images.size(); ++i)
    {
        ImageProxy& proxy = this->images[i];
        if (proxy.is_dirty || proxy.image.use_count() != 1)
            continue;
        proxy.image.reset();
        released += 1;
    }
    for (std::size_t i = 0; i < this->blobs.size(); ++i)
    {
        BlobProxy& proxy = this->blobs[i];
        if (proxy.is_dirty || proxy.blob.use_count() != 1)
            continue;
        proxy.blob.reset();
        released += 1;
    }
    return released;
}

std::size_t
View::get_byte_size (void) const
{
    std::size_t ret = 0;
    for (std::size_t i = 0; i < this->images.size(); ++i)
        if (this->images[i].image != NULL)
            ret += this->images[i].image->get_byte_size();
    for (std::size_t i = 0; i < this->blobs.size(); ++i)
        if (this->blobs[i].blob != NULL)
            ret += this->blobs[i].blob->get_byte_size();
    return ret;
}

/* ---------------------------------------------------------------- */

std::string
View::get_value (std::string const& key) const
{
    if (key.empty())
        throw std::invalid_argument("Empty key");
    if (key.find_first_of('.') == std::string::npos)
        throw std::invalid_argument("Missing section identifier");
    typedef MetaData::KeyValueMap::const_iterator KeyValueIter;
    KeyValueIter iter = this->meta_data.data.find(key);
    if (iter == this->meta_data.data.end())
        return std::string();
    return iter->second;
}

void
View::set_value (std::string const& key, std::string const& value)
{
    if (key.empty())
        throw std::invalid_argument("Empty key");
    if (key.find_first_of('.') == std::string::npos)
        throw std::invalid_argument("Missing section identifier");
    this->meta_data.data[key] = value;
    this->meta_data.is_dirty = true;
}

void
View::delete_value (std::string const& key)
{
    this->meta_data.data.erase(key);
}

void
View::set_camera (CameraInfo const& camera)
{
    this->meta_data.camera = camera;
    this->meta_data.is_dirty = true;

    /* Re-generate the "camera" section. */
    this->set_value("camera.focal_length", util::string::get(camera.flen));
    this->set_value("camera.pixel_aspect", util::string::get(camera.paspect));
    this->set_value("camera.principal_point",
        util::string::get(camera.ppoint[0]) + " "
        + util::string::get(camera.ppoint[1]));
    this->set_value("camera.rotation", camera.get_rotation_string());
    this->set_value("camera.translation", camera.get_translation_string());
}

/* ---------------------------------------------------------------- */

ImageBase::Ptr
View::get_image (std::string const& name, ImageType type)
{
    View::ImageProxy* proxy = this->find_image_intern(name);
    if (proxy != NULL)
    {
        if (type == IMAGE_TYPE_UNKNOWN)
            return this->load_image(proxy, false);
        this->initialize_image(proxy, false);
        if (proxy->type == type)
            return this->load_image(proxy, false);
    }
    return ImageBase::Ptr();
}

View::ImageProxy const*
View::get_image_proxy (std::string const& name, ImageType type)
{
    View::ImageProxy* proxy = this->find_image_intern(name);
    if (proxy != NULL)
    {
        this->initialize_image(proxy, false);
        if (type == IMAGE_TYPE_UNKNOWN || proxy->type == type)
            return proxy;
    }
    return NULL;
}

bool
View::has_image (std::string const& name, ImageType type)
{
    View::ImageProxy* proxy = this->find_image_intern(name);
    if (proxy == NULL)
        return false;
    if (type == IMAGE_TYPE_UNKNOWN)
        return true;
    this->initialize_image(proxy, false);
    return proxy->type == type;
}

void
View::add_image (ImageBase::Ptr image, std::string const& name)
{
    if (image == NULL)
        throw std::invalid_argument("NULL image");

    ImageProxy const* tmp = this->get_image_proxy(name);
    if (tmp != NULL)
        throw util::Exception("Image already exists: ", name);

    this->set_image(image, name);
}

void
View::set_image (ImageBase::Ptr image, std::string const& name)
{
    if (image == NULL)
        throw std::invalid_argument("NULL image");

    ImageProxy* old = NULL;
    for (std::size_t i = 0; old == NULL && i < this->images.size(); ++i)
        if (this->images[i].name == name)
            old = &this->images[i];

    ImageProxy proxy;
    proxy.is_dirty = true;
    proxy.name = name;
    proxy.is_initialized = true;
    proxy.width = image->width();
    proxy.height = image->height();
    proxy.channels = image->channels();
    proxy.type = image->get_type();
    proxy.image = image;

    if (old == NULL)
        this->images.push_back(proxy);
    else
        *old = proxy;
}

bool
View::remove_image (std::string const& name)
{
    View::ImageProxy* proxy = this->find_image_intern(name);
    if (proxy == NULL)
        return false;

    /* Clear the name to indicate file deletion on save. */
    proxy->name.clear();
    proxy->image.reset();
    proxy->is_dirty = true;
    return true;
}

/* ---------------------------------------------------------------- */

ByteImage::Ptr
View::get_blob (std::string const& name)
{
    BlobProxy* proxy = this->find_blob_intern(name);
    if (proxy != NULL)
        return this->load_blob(proxy, false);
    return ByteImage::Ptr();
}

View::BlobProxy const*
View::get_blob_proxy (std::string const& name)
{
    BlobProxy* proxy = this->find_blob_intern(name);
    if (proxy != NULL)
        this->initialize_blob(proxy, false);
    return proxy;
}

bool
View::has_blob (std::string const& name)
{
    return this->find_blob_intern(name) != NULL;
}

void
View::add_blob (ByteImage::Ptr blob, std::string const& name)
{
    if (blob == NULL)
        throw std::invalid_argument("NULL blob");

    BlobProxy const* tmp = this->get_blob_proxy(name);
    if (tmp != NULL)
        throw util::Exception("Blob already exists: ", name);

    this->set_blob(blob, name);
}

void
View::set_blob (ByteImage::Ptr blob, std::string const& name)
{
    if (blob == NULL)
        throw std::invalid_argument("NULL blob");

    BlobProxy* old = NULL;
    for (std::size_t i = 0; old == NULL && i < this->blobs.size(); ++i)
        if (this->blobs[i].name == name)
            old = &this->blobs[i];

    BlobProxy proxy;
    proxy.is_dirty = true;
    proxy.name = name;
    proxy.is_initialized = true;
    proxy.size = blob->get_byte_size();
    proxy.blob = blob;

    if (old == NULL)
        this->blobs.push_back(proxy);
    else
        *old = proxy;
}

bool
View::remove_blob (std::string const& name)
{
    View::BlobProxy* proxy = this->find_blob_intern(name);
    if (proxy == NULL)
        return false;

    /* Clear the name to indicate file deletion on save. */
    proxy->name.clear();
    proxy->blob.reset();
    proxy->is_dirty = true;
    return true;
}

/* ------------------------ Private Members ----------------------- */

void
View::load_meta_data (std::string const& path)
{
    this->parse_meta_data_file(path);

    /* Get camera data from key/value pairs. */
    std::string cam_fl = this->get_value("camera.focal_length");
    std::string cam_pa = this->get_value("camera.pixel_aspect");
    std::string cam_pp = this->get_value("camera.principal_point");
    std::string cam_rot = this->get_value("camera.rotation");
    std::string cam_trans = this->get_value("camera.translation");

    this->meta_data.camera = CameraInfo();
    if (!cam_fl.empty())
        this->meta_data.camera.flen = util::string::convert<float>(cam_fl);
    if (!cam_pa.empty())
        this->meta_data.camera.paspect = util::string::convert<float>(cam_pa);
    if (!cam_pp.empty())
    {
        std::stringstream ss(cam_pp);
        ss >> this->meta_data.camera.ppoint[0];
        ss >> this->meta_data.camera.ppoint[1];
    }
    if (!cam_rot.empty())
        this->meta_data.camera.set_rotation_from_string(cam_rot);
    if (!cam_trans.empty())
        this->meta_data.camera.set_translation_from_string(cam_trans);
}

void
View::parse_meta_data_file (std::string const& path)
{
    std::string const fname = util::fs::join_path(path, VIEW_IO_META_FILE);
    this->meta_data.is_dirty = false;

    /* Open meta file and read key/value pairs. */
    std::ifstream in(fname.c_str());
    if (!in.good())
        throw util::FileException(fname, "Error opening");

    /* Parse INI file. */
    std::string line;
    std::string section_name;
    std::size_t line_number = 0;
    std::stringstream failure_message;
    while (true)
    {
        std::getline(in, line);
        if (in.eof())
            break;

        line_number += 1;
        util::string::clip_newlines(&line);
        util::string::clip_whitespaces(&line);
        if (line.empty())
            continue;
        if (line[0] == '#')
            continue;

        if (line[0] == '[' && line[line.size() - 1] == ']')
        {
            section_name = line.substr(1, line.size() - 2);
            continue;
        }

        std::size_t sep_pos = line.find_first_of('=');
        if (sep_pos != std::string::npos)
        {
            std::string key = line.substr(0, sep_pos);
            util::string::clip_newlines(&key);
            util::string::clip_whitespaces(&key);

            std::string value = line.substr(sep_pos + 1);
            util::string::clip_newlines(&value);
            util::string::clip_whitespaces(&value);

            if (key.empty())
            {
                failure_message << "Line " << line_number << ": Empty key";
                break;
            }

            if (section_name.empty())
            {
                failure_message << "Line " << line_number << ": No section";
                break;
            }

            key = section_name + "." + key;
            this->meta_data.data[key] = value;
            continue;
        }

        failure_message << "Line " << line_number << ": Invalid line";
        break;
    }

    /* Close input file. */
    in.close();

    /* Check for errors to report... */
    std::string failure_string = failure_message.str();
    if (!failure_string.empty())
        throw util::Exception(failure_string);
}

void
View::save_meta_data (std::string const& path)
{
    std::cout << "View: Saving meta data to " VIEW_IO_META_FILE << std::endl;
    std::string const fname = util::fs::join_path(path, VIEW_IO_META_FILE);
    std::string const fname_new = fname + ".new";

    /* Write meta data to file. */
    std::ofstream out(fname_new.c_str());
    if (!out.good())
        throw util::FileException(fname_new, std::strerror(errno));

    out << "# MVE view meta data is stored in INI-file syntax.\n";
    out << "# This file is generated, formatting will get lost.\n";

    std::string last_section;
    MetaData::KeyValueMap::const_iterator iter;
    for (iter = this->meta_data.data.begin();
        iter != this->meta_data.data.end(); iter++)
        {
            std::string key = iter->first;
            std::string value = iter->second;
            util::string::clip_newlines(&key);
            util::string::clip_whitespaces(&key);
            util::string::clip_newlines(&value);
            util::string::clip_whitespaces(&value);

            std::size_t section_pos = key.find_first_of('.');
            if (section_pos == std::string::npos)
                throw std::runtime_error("Key/value pair without section");
            std::string section = key.substr(0, section_pos);
            key = key.substr(section_pos + 1);

            if (section != last_section)
            {
                out << "\n[" << section << "]\n";
                last_section = section;
            }

            out << key << " = " << value << std::endl;
        }

    out.close();

    /* On succesfull write, move the new file in place. */
    this->replace_file(fname, fname_new);
    this->meta_data.is_dirty = false;
}

void
View::populate_images_and_blobs (std::string const& path)
{
    util::fs::Directory dir(path);
    for (std::size_t i = 0; i < dir.size(); ++i)
    {
        util::fs::File const& file = dir[i];
        if (file.name == VIEW_IO_META_FILE)
            continue;

        std::string ext4 = util::string::right(file.name, 4);
        std::string ext5 = util::string::right(file.name, 5);
        ext4 = util::string::lowercase(ext4);
        ext5 = util::string::lowercase(ext5);

        if (ext4 == ".png" || ext4 == ".jpg" ||
            ext5 == ".jpeg" || ext5 == ".mvei")
        {
            std::cout << "View: Adding image proxy: "
                << file.name << std::endl;

            ImageProxy proxy;
            proxy.is_dirty = false;
            proxy.filename = file.name;
            proxy.name = file.name.substr(0, file.name.find_last_of('.'));
            this->images.push_back(proxy);
        }
        else if (ext5 == ".blob")
        {
            std::cout << "View: Adding BLOB proxy "
                << file.name << std::endl;

            BlobProxy proxy;
            proxy.is_dirty = false;
            proxy.filename = file.name;
            proxy.name = file.name.substr(0, file.name.find_last_of('.'));
            this->blobs.push_back(proxy);
        }
        else
        {
            std::cerr << "View: Unrecognized file "
                << file.name << ", skipping." << std::endl;
        }
    }
}

void
View::replace_file (std::string const& old_fn, std::string const& new_fn)
{
    /* Delete old file. */
    if (util::fs::file_exists(old_fn.c_str()))
        if (!util::fs::unlink(old_fn.c_str()))
            throw util::FileException(old_fn, std::strerror(errno));

    /* Rename new file. */
    if (!util::fs::rename(new_fn.c_str(), old_fn.c_str()))
        throw util::FileException(new_fn, std::strerror(errno));
}

/* ---------------------------------------------------------------- */

View::ImageProxy*
View::find_image_intern (std::string const& name)
{
    if (name.empty())
        return NULL;
    for (std::size_t i = 0; i < this->images.size(); ++i)
        if (this->images[i].name == name)
            return &this->images[i];
    return NULL;
}

void
View::initialize_image (ImageProxy* proxy, bool update)
{
    if (proxy->is_initialized && !update)
        return;
    this->load_image_intern(proxy, true);
}

ImageBase::Ptr
View::load_image (ImageProxy* proxy, bool update)
{
    if (proxy->image != NULL && !update)
        return proxy->image;
    this->load_image_intern(proxy, false);
    return proxy->image;
}

void
View::load_image_intern (ImageProxy* proxy, bool init_only)
{
    if (this->path.empty())
        throw std::runtime_error("View not initialized");
    if (proxy->name.empty())
        return;

    /* Load image and update meta data. */
    std::string filename = util::fs::join_path(this->path, proxy->filename);
    std::cout << "View: Loading image " << filename << std::endl;

    if (init_only)
    {
        image::ImageHeaders headers = image::load_file_headers(filename);
        proxy->is_dirty = false;
        proxy->width = headers.width;
        proxy->height = headers.height;
        proxy->channels = headers.channels;
        proxy->type = headers.type;
        proxy->is_initialized = true;
        return;
    }

    std::string ext4 = util::string::right(proxy->filename, 4);
    std::string ext5 = util::string::right(proxy->filename, 5);
    ext4 = util::string::lowercase(ext4);
    ext5 = util::string::lowercase(ext5);

    if (ext4 == ".png" || ext4 == ".jpg" || ext5 == ".jpeg")
    {
        //std::cout << "View: Loading 8-bit image...";
        proxy->image = image::load_file(filename);
    }
    else if (ext5 == ".mvei")
    {
        //std::cout << "View: Loading MVE image...";
        proxy->image = image::load_mvei_file(filename);
    }
    else
    {
        throw std::runtime_error("Unexpected image type");
    }

    proxy->is_dirty = false;
    proxy->width = proxy->image->width();
    proxy->height = proxy->image->height();
    proxy->channels = proxy->image->channels();
    proxy->type = proxy->image->get_type();
    proxy->is_initialized = true;
}

void
View::save_image_intern (ImageProxy* proxy)
{
    /* An empty name indicates a delete request. */
    if (proxy->name.empty() && !proxy->filename.empty())
    {
        std::string fname = util::fs::join_path(this->path, proxy->filename);
        if (util::fs::file_exists(fname.c_str())
            && !util::fs::unlink(fname.c_str()))
            throw util::FileException(fname, std::strerror(errno));
        proxy->is_dirty = false;
        return;
    }

    if (this->path.empty())
        throw std::runtime_error("View not initialized");
    if (proxy == NULL || proxy->image == NULL)
        throw std::runtime_error("NULL image");
    if (proxy->width != proxy->image->width()
        || proxy->height != proxy->image->height()
        || proxy->channels != proxy->image->channels()
        || proxy->type != proxy->image->get_type())
        throw std::runtime_error("Image specification mismatch");

    /* Generate a new filename for the image. */
    bool use_png_format = false;
    if (proxy->image->get_type() == IMAGE_TYPE_UINT8
        && proxy->image->channels() <= 4)
        use_png_format = true;

    std::string filename = proxy->name + (use_png_format ? ".png" : ".mvei");
    std::string fname_orig = util::fs::join_path(this->path, proxy->filename);
    std::string fname_save = util::fs::join_path(this->path, filename);
    std::string fname_new = fname_save + ".new";

    /* Save the new image. */
    std::cout << "View: Saving image " << filename << std::endl;
    if (use_png_format)
        image::save_png_file(proxy->image, fname_new);
    else
        image::save_mvei_file(proxy->image, fname_new);

    /* On succesfull write, move the new file in place. */
    this->replace_file(fname_save, fname_new);

    /* If the original file was different (e.g. JPG), remove it. */
    if (fname_save != fname_orig
        && util::fs::file_exists(fname_orig.c_str())
        && !util::fs::unlink(fname_orig.c_str()))
        throw util::FileException(fname_orig, std::strerror(errno));

    /* Fully update the proxy. */
    proxy->is_dirty = false;
    proxy->filename = filename;
    proxy->width = proxy->image->width();
    proxy->height = proxy->image->height();
    proxy->channels = proxy->image->channels();
    proxy->type = proxy->image->get_type();
    proxy->is_initialized = true;
}

/* ---------------------------------------------------------------- */

View::BlobProxy*
View::find_blob_intern (std::string const& name)
{
    if (name.empty())
        return NULL;
    for (std::size_t i = 0; i < this->blobs.size(); ++i)
        if (this->blobs[i].name == name)
            return &this->blobs[i];
    return NULL;
}

void
View::initialize_blob (BlobProxy* proxy, bool update)
{
    if (proxy->is_initialized && !update)
        return;
    this->load_blob_intern(proxy, true);
}

ByteImage::Ptr
View::load_blob (BlobProxy* proxy, bool update)
{
    if (proxy->blob != NULL && !update)
        return proxy->blob;
    this->load_blob_intern(proxy, false);
    return proxy->blob;
}

void
View::load_blob_intern (BlobProxy* proxy, bool init_only)
{
    if (this->path.empty())
        throw std::runtime_error("View not initialized");
    if (proxy->name.empty())
        return;

    /* Load blob and update meta data. */
    std::string filename = util::fs::join_path(this->path, proxy->filename);
    std::cout << "View: Loading BLOB " << filename << std::endl;

    std::ifstream in(filename.c_str(), std::ios::binary);
    if (!in.good())
        throw util::FileException(filename, std::strerror(errno));

    /* Read file signature. */
    char signature[VIEW_IO_BLOB_SIGNATURE_LEN];
    in.read(signature, VIEW_IO_BLOB_SIGNATURE_LEN);
    if (!std::equal(signature, signature + VIEW_IO_BLOB_SIGNATURE_LEN,
        VIEW_IO_BLOB_SIGNATURE))
        throw util::Exception("Invalid BLOB file signature");

    /* Read blob size. */
    uint64_t size;
    in.read(reinterpret_cast<char*>(&size), sizeof(uint64_t));
    if (!in.good())
        throw util::FileException(filename, "EOF while reading BLOB headers");

    if (init_only)
    {
        proxy->size = size;
        proxy->is_initialized = true;
        return;
    }

    /* Read blob payload. */
    ByteImage::Ptr blob = ByteImage::create(size, 1, 1);
    in.read(blob->get_byte_pointer(), blob->get_byte_size());
    if (!in.good())
        throw util::FileException(filename, "EOF while reading BLOB payload");

    proxy->blob = blob;
    proxy->size = size;
    proxy->is_initialized = true;
}

void
View::save_blob_intern (BlobProxy* proxy)
{
    /* An empty name indicates a delete request. */
    if (proxy->name.empty() && !proxy->filename.empty())
    {
        std::string fname = util::fs::join_path(this->path, proxy->filename);
        if (util::fs::file_exists(fname.c_str())
            && !util::fs::unlink(fname.c_str()))
            throw util::FileException(fname, std::strerror(errno));
        proxy->is_dirty = false;
        return;
    }

    if (this->path.empty())
        throw std::runtime_error("View not initialized");
    if (proxy == NULL || proxy->blob == NULL)
        throw std::runtime_error("NULL BLOB");
    if (proxy->blob->get_byte_size() != proxy->size)
        throw std::runtime_error("BLOB size mismatch");

    /* If the object has never been saved, the filename is empty. */
    if (proxy->filename.empty())
        proxy->filename = proxy->name + ".blob";

    /* Create a .new file and save blob. */
    std::string fname_orig = util::fs::join_path(this->path, proxy->filename);
    std::string fname_new = fname_orig + ".new";

    // Check if file exists? Create unique temp name?
    std::cout << "View: Saving BLOB " << proxy->filename << std::endl;
    std::ofstream out(fname_new.c_str());
    if (!out.good())
        throw util::FileException(fname_new, std::strerror(errno));

    out.write(VIEW_IO_BLOB_SIGNATURE, VIEW_IO_BLOB_SIGNATURE_LEN);
    out.write(reinterpret_cast<char const*>(&proxy->size), sizeof(uint64_t));
    out.write(proxy->blob->get_byte_pointer(), proxy->blob->get_byte_size());
    if (!out.good())
        throw util::FileException(fname_new, std::strerror(errno));
    out.close();

    /* On succesfull write, move the new file in place. */
    this->replace_file(fname_orig, fname_new);

    /* Fully update the proxy. */
    proxy->is_dirty = false;
    proxy->size = proxy->blob->get_byte_size();
    proxy->is_initialized = true;
}

/* ---------------------------------------------------------------- */

void
View::debug_print (void)
{
    for (std::size_t i = 0; i < this->images.size(); ++i)
        this->initialize_image(&this->images[i], false);
    for (std::size_t i = 0; i < this->blobs.size(); ++i)
        this->initialize_blob(&this->blobs[i], false);

    std::cout << std::endl;
    std::cout << "Path: " << this->path << std::endl;
    std::cout << "View Name: " << this->get_value("view.name") << std::endl;
    std::cout << "View key/value pairs:" << std::endl;

    typedef MetaData::KeyValueMap::const_iterator KeyValueIter;
    for (KeyValueIter iter = this->meta_data.data.begin();
        iter != this->meta_data.data.end(); iter++)
        std::cout << "  " << iter->first << " = " << iter->second << std::endl;

    std::cout << "View images:" << std::endl;
    for (std::size_t i = 0; i < this->images.size(); ++i)
    {
        ImageProxy const& proxy = this->images[i];
        std::cout << "  " << proxy.name << " (" << proxy.filename << ")"
            << ", size " << proxy.width << "x" << proxy.height << "x" << proxy.channels
            << ", type " << proxy.type << std::endl;
    }

    std::cout << "View BLOBs:" << std::endl;
    for (std::size_t i = 0; i < this->blobs.size(); ++i)
    {
        BlobProxy const& proxy = this->blobs[i];
        std::cout << "  " << proxy.name << " (" << proxy.filename << ")"
            << ", size " << proxy.size << std::endl;
    }
}

MVE_NAMESPACE_END
