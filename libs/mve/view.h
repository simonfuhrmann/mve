/*
 * A reader, writer and API for the MVE file format.
 * Written by Simon Fuhrmann.
 *
 * The implementation is aimed at process safety and partial thread safety.
 * For process safety, whenever a file is read or written to, a file lock is
 * acquired. Whenever a file is read from, the headers are re-read to ensure
 * a consistent state of the view across processes. For thread safety, a
 * mutexed access to the embeddings is implemented, to ensure that embeddings
 * are load only once.
 *
 * Current limitations:
 * - The following data types are supported:
 *   uint8, uint16, float, double, sint32
 * - Unexpected behavior occures if a process changes the structure of an
 *   MVE file while another process relies on the previously read header.
 *
 * TODO: New Features
 * - Merging
 * - Re-reading and merging before some operations
 */

#ifndef MVE_VIEW_HEADER
#define MVE_VIEW_HEADER

#include <string>
#include <vector>

#include "util/ref_ptr.h"
#include "util/atomic.h"
#include "mve/defines.h"
#include "mve/camera.h"
#include "mve/image_base.h"
#include "mve/image.h"

MVE_NAMESPACE_BEGIN

/**
 * Proxy class for image and data proxies.
 * For data proxies, 'is_image' is set to false, width is set to the
 * byte size of the embedding, height and channels is set to 1.
 */
struct MVEFileProxy
{
    /* Properties that can be expected to be always set and valid. */
    std::string name; ///< Name of the embedding
    mve::ImageBase::Ptr image; ///< The actual image data if cached
    bool is_image; ///< Indicator whether embedding is an image or data
    bool is_dirty; ///< Indicates that embedding has changed

    /* Image/data properties as present in file. */
    int width; ///< Width of image (or length of data).
    int height; ///< Height of image (or 1 for data).
    int channels; ///< Channels of image (or 1 for data).
    std::string datatype; ///< String rep. of image datatype.

    /* Properties that links the embedding to a storage location. */
    std::size_t byte_size; ///< Size of the embedding
    std::size_t file_pos; ///< Position of the embedding within the file

    MVEFileProxy (void);
    bool check_direct_write (void) const;
    ImageType get_type (void) const;
    bool is_type (std::string const& typestr) const;
};

/* ---------------------------------------------------------------- */

/**
 * MVE file per-view meta data.
 */
struct MVEFileMeta
{
    /** ID of the view. */
    std::size_t view_id;

    /** Name of the view, typically zero-padded ID of the view. */
    std::string view_name;

    /** Extrinsic camera parameters, t1 .. t3 r1 .. r9. */
    std::string camera_ext_str;

    /**
     * Intrinsic camera parameters, flen rd1 rd2 pa ppx ppy.
     * This is: focal length, radial distortions, pixel aspect ratio,
     * and finally the principal point in x and y direction.
     */
    std::string camera_int_str;

    MVEFileMeta (void);
};

/* ---------------------------------------------------------------- */

/**
 * Implementation of the MVE file specification to read and write MVE files.
 *
 * MVE files are per-view containers that contain per-view meta data (like
 * camera parameters) and an arbitrary amount of image and data embeddings.
 * Images may be of arbitrary type, data embeddings may be binary.
 */
class View
{
public:
    typedef util::RefPtr<View> Ptr;
    typedef util::RefPtr<View const> ConstPtr;
    typedef std::vector<MVEFileProxy> Proxies;

public:
    static View::Ptr create (void);
    static View::Ptr create (std::string const& filename);

    /* ----------------------- Manage view ------------------------ */

    /** Sets the view ID. */
    void set_id (std::size_t view_id);
    /** Sets the name of the view. */
    void set_name (std::string const& name);
    /** Sets camera information of the view. */
    void set_camera (CameraInfo const& camera);

    /** Returns the ID of the view. */
    std::size_t get_id (void) const;
    /** Returns the name of the view. */
    std::string const& get_name (void) const;
    /** Returns the camera information of the view. */
    CameraInfo const& get_camera (void) const;
    /** Returns the filename the view is connected with. */
    std::string const& get_filename (void) const;
    /** Returns true if the camera is valid. */
    bool is_camera_valid (void) const;
    /** Returns true if embeddings are dirty or view attribs changed. */
    bool is_dirty (void) const;

    /** Clears the view. */
    void clear (void);

    /** Returns the memory consumption in bytes. */
    std::size_t get_byte_size (void) const;

    /** Cleans unused embeddings and returns amount of cleaned embeddings. */
    std::size_t cache_cleanup (void);

    /* ------------------ Managing of embeddings ------------------ */

    /**
     * Provides read-only access to a single proxy by name.
     * It is unsafe to store this pointer for later use, as proxies
     * may be deleted or relocated in memory when the view is updated.
     */
    MVEFileProxy const* get_proxy (std::string const& name) const;

    /**
     * Provides read-only access to the underlying list of proxies.
     */
    Proxies const& get_proxies (void) const;

    /**
     * Returns an embedding by name. Returns a NULL pointer if the
     * embedding does not exist.
     */
    mve::ImageBase::Ptr get_embedding (std::string const& name);

    /**
     * Returns true if an embedding by the given name exists.
     */
    bool has_embedding (std::string const& name) const;

    /**
     * Returns true if an image embedding by the given name exists.
     */
    bool has_image_embedding (std::string const& name) const;

    /**
     * Returns true if a data embedding by the given name exists.
     */
    bool has_data_embedding (std::string const& name) const;

    /**
     * Returns true if the embedding by that name has been removed.
     * If more than one embedding by that name exist, it deletes all.
     */
    bool remove_embedding (std::string const& name);

    /**
     * Returns the amount of embeddings (image and data embeddings).
     */
    std::size_t num_embeddings (void) const;

    /**
     * Returns the amount of image embeddings. This requries counting.
     */
    std::size_t count_image_embeddings (void) const;

    /**
     * Returns true if the embedding by that name has been marked dirty.
     */
    bool mark_as_dirty (std::string const& name);

    /* -------------------- Managing of images -------------------- */

    /**
     * Adds an image embedding to the view. If an embedding by that
     * name already exists, an exception is raised.
     */
    void add_image (std::string const& name, ImageBase::Ptr image);

    /**
     * Sets an image embedding to the view and marks the embedding dirty.
     * If an embedding by that name already exists, it is overwritten.
     */
    void set_image (std::string const& name, ImageBase::Ptr image);

    /**
     * Returns an image embedding as generic type image.
     * Note that changing this image also changes the view contents.
     * After image contents has been modified, mark the embedding dirty!
     */
    ImageBase::Ptr get_image (std::string const& name);

    /**
     * Returns a float image by name or 0 on error.
     * Note that changing this image also changes the view contents.
     * After image contents has been modified, mark the embedding dirty!
     */
    FloatImage::Ptr get_float_image (std::string const& name);

    /**
     * Returns a double image by name or 0 on error.
     * Note that changing this image also changes the view contents.
     * After image contents has been modified, mark the embedding dirty!
     */
    DoubleImage::Ptr get_double_image (std::string const& name);

    /**
     * Returns a byte image by name or 0 on error.
     * Note that changing this image also changes the view contents.
     * After image contents has been modified, mark the embedding dirty!
     */
    ByteImage::Ptr get_byte_image (std::string const& name);

    /**
     * Returns an integer image by name or 0 on error.
     * Note that changing this image also changes the view contents.
     * After image contents has been modified, mark the embedding dirty!
     */
    IntImage::Ptr get_int_image (std::string const& name);

    /* --------------- Managing of data embeddings ---------------- */

    /**
     * Adds a data embedding to the view. If an embedding by that
     * name already exists, an exception is raised.
     */
    void add_data (std::string const& name, ByteImage::Ptr data);

    /**
     * Sets a data embedding to the view and marks the embedding dirty.
     * If an embedding by that name already exists, it is overwritten.
     */
    void set_data (std::string const& name, ByteImage::Ptr data);

    /**
     * Returns a data embedding as byte image.
     * Note that changing this image also changes the view contents.
     * After contents has been modified, mark the embedding dirty!
     */
    ByteImage::Ptr get_data (std::string const& name);

    /* ------------ MVE file read and write interface. ------------ */

    /**
     * Loads the MVE file by parsing the headers and populating embeddings.
     * If merge is requested, cached and dirty embeddings are kept. A merge
     * is performed every time the associated file is accessed and has changed.
     */
    void load_mve_file (std::string const& filename, bool merge = false);

    /** Loads the MVE file using the associated filename. */
    void reload_mve_file (bool merge = false);

    /** Writes view to given file, sets filename. */
    void save_mve_file_as (std::string const& filename);

    /** Writes view to default filename. */
    void save_mve_file (bool force_rebuild = false);

    /**
     * Renames the file physically and changes associated filename.
     * Returns false if renaming operation failed, however, the
     * associated filename is still changed.
     */
    bool rename_file (std::string const& new_name);

    /** Debug output. */
    void print_debug (void) const;

protected:
    /** Default constructor. */
    View (void);

    /** Constructor that immediately loads the MVE file. */
    View (std::string const& filename);

private:
    void parse_header_line (std::string const& header_line);
    void direct_write (MVEFileProxy& proxy);
    ImageBase::Ptr get_image_for_proxy (MVEFileProxy& proxy);
    MVEFileProxy* get_proxy_intern (std::string const& name);
    void update_camera (void);

private:
    std::string filename; ///< Filename the view is associated with
    MVEFileMeta meta; ///< Meta information, view name, camera, etc
    CameraInfo camera; ///< Per-view camera information
    Proxies proxies; ///< Proxies for all embeddings
    bool needs_rebuild; ///< Disables direct-writing when saving
    util::Atomic<int> loading_mutex; ///< Mutex to guard file access
};

/* ---------------------------------------------------------------- */

inline
MVEFileProxy::MVEFileProxy (void)
    : is_image(true)
    , is_dirty(false)
    , width(0)
    , height(0)
    , channels(0)
    , byte_size(0)
    , file_pos(0)
{
}

inline
MVEFileMeta::MVEFileMeta (void)
    : view_id(static_cast<std::size_t>(-1))
{
}

inline
View::View (void)
    : needs_rebuild(false)
    , loading_mutex(0)
{
}

inline
View::View (std::string const& fname)
    : needs_rebuild(false)
    , loading_mutex(0)
{
    this->load_mve_file(fname);
}

inline View::Ptr
View::create (void)
{
    return Ptr(new View());
}

inline View::Ptr
View::create (std::string const& filename)
{
    return Ptr(new View(filename));
}

inline void
View::set_id (std::size_t view_id)
{
    if (this->meta.view_id != view_id)
        this->needs_rebuild = true;
    this->meta.view_id = view_id;
}

inline void
View::set_name (std::string const& name)
{
    if (this->meta.view_name != name)
        this->needs_rebuild = true;
    this->meta.view_name = name;
}

inline std::size_t
View::get_id (void) const
{
    return this->meta.view_id;
}

inline std::string const&
View::get_name (void) const
{
    return this->meta.view_name;
}

inline CameraInfo const&
View::get_camera (void) const
{
    return this->camera;
}

inline std::string const&
View::get_filename (void) const
{
    return this->filename;
}

inline bool
View::is_camera_valid (void) const
{
    return this->camera.flen != 0.0f;
}

inline void
View::clear (void)
{
    this->meta = MVEFileMeta();
    this->camera = CameraInfo();
    this->filename.clear();
    this->proxies.clear();
    this->needs_rebuild = false;
}

inline bool
View::has_embedding (std::string const& name) const
{
    return this->get_proxy(name) != NULL;
}

inline bool
View::has_image_embedding (std::string const& name) const
{
    MVEFileProxy const* proxy = this->get_proxy(name);
    return proxy != NULL && proxy->is_image == true;
}

inline bool
View::has_data_embedding (std::string const& name) const
{
    MVEFileProxy const* proxy = this->get_proxy(name);
    return proxy != NULL && proxy->is_image == false;
}

inline std::size_t
View::num_embeddings (void) const
{
    return this->proxies.size();
}

inline bool
View::mark_as_dirty (std::string const& name)
{
    MVEFileProxy* p(this->get_proxy_intern(name));
    if (p != NULL)
        p->is_dirty = true;
    return p != NULL;
}

inline void
View::reload_mve_file (bool merge)
{
    this->load_mve_file(this->filename, merge);
}

inline View::Proxies const&
View::get_proxies (void) const
{
    return this->proxies;
}

MVE_NAMESPACE_END

#endif /* MVE_VIEW_HEADER */
