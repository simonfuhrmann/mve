/*
 * File system abstraction for common functions.
 * Written by Andre Schulz and Simon Fuhrmann.
 */

#ifndef UTIL_FS_HEADER
#define UTIL_FS_HEADER

#include <string>
#include <vector>

#include "util/defines.h"

UTIL_NAMESPACE_BEGIN
UTIL_FS_NAMESPACE_BEGIN

/*
 * -------------------------- Path operations ------------------------
 */

/** Determines if the given path is a directory. */
bool dir_exists (char const* pathname);
/** Determines if the given path is a file. */
bool file_exists (char const* pathname);
/** Determines the home path for the current user. */
char* get_default_home_path (void);
/** Determines the current working directory of the process. */
char* get_cwd (char* buf, std::size_t size);
/** Changes working directory to 'pathname', returns true on success. */
bool set_cwd (char const* pathname);

/*
 * -------------------------- File operations ------------------------
 */

// TODO: define modes for mkdir() for all platforms?
/** Creates a new directory. */
bool mkdir (char const* pathname/*, mode_t mode*/);
/** Unlinks (deletes) the given file. */
bool unlink (char const* pathname);
/** Renames the given file 'from' to new name 'to'. */
bool rename (char const* from, char const* to);
///** Creates an empty file. */
//http://www.koders.com/c/fid96336B4591FD0C5D2C4DADA2D264D3A04F21A934.aspx
//bool touch (char const* pathname);

/*
 * ------------------------- String processing -----------------------
 */

/** Determines the CWD and returns a convenient string. */
std::string get_cwd_string (void);
/** Returns the path of the binary currently executing. */
std::string get_binary_path (void);
/** Returns the absolute base path component of 'path'. */
std::string get_path_component (std::string const& path);
/** Returns the local file component if the given 'path'. */
std::string get_file_component (std::string const& path);
/** Replaces extension of the given file with 'ext'. */
std::string replace_extension (std::string const& fn, std::string const& ext);

/*
 * ------------------------- File abstraction ------------------------
 */

struct File
{
    std::string path;
    std::string name;
    bool is_dir;

    File (void);
    File (std::string const& path, std::string const& name, bool isdir = false);
    std::string get_absolute_name (void) const;
    bool operator< (File const& rhs) const;
};

/*
 * ------------------------- Directory reading -----------------------
 */

/** Directory abstraction to scan directory contents. */
class Directory : public std::vector<File>
{
public:
    Directory (void);
    Directory (std::string const& path);
    void scan (std::string const& path);
};

/*
 * --------------------- File locking mechanism ----------------------
 */

/**
 * A simple file-based file lock implementation.
 * A file NAME.lock is created when a lock is acquired, and
 * the file is removed when the lock is released. If the lock
 * already exist, 'acquire' returns false. 'acquire_retry' retries for
 * a few seconds and finally gives up returning false. Note that the ".lock"
 * suffix is automatically used and must not be part of the given filenames.
 */
class FileLock
{
private:
    std::string lockfile;
    std::string reason;

public:
    /** Does nothing. */
    FileLock (void);
    /** Acquires a lock for the given filename. */
    FileLock (std::string const& filename);
    /** Removes the lock if it exists. */
    ~FileLock (void);

    /**
     * Tries to acquire a lock for the given filename.
     * If locking fails, the method returns false and specifies a reason.
     */
    bool acquire (std::string const& filename);

    /**
     * Tries to acquire a lock for the given filename.
     * If a lock exists, the operation is re-attempted 'retry' times
     * with 'sleep' milli seconds delay between the attempts.
     * If locking fails, the method returns false and specifies a reason.
     */
    bool acquire_retry (std::string const& filename,
        int retries = 50, int sleep = 100);

    /**
     * Returns true if a lock for the given filename exists.
     */
    bool is_locked (std::string const& filename);

    /**
     * Waits until a lock for given filename is released.
     * If the given filename is not locked, the method returns immediately.
     * If the lock is not released within the specified bounds, the method
     * returns false and specifies a reason.
     */
    bool wait_lock (std::string const& filename,
        int retries = 50, int sleep = 100);

    /**
     * Removes the lock if it exists.
     * If removing the lock fails, the method returns false.
     */
    bool release (void);

    /**
     * If locking failes, this returns the reason for failure.
     */
    std::string const& get_reason (void) const;
};

/*
 * -------------------------- Implementation -------------------------
 */

inline
File::File (void)
    : is_dir(false)
{
}

inline
File::File (std::string const& path, std::string const& name, bool isdir)
    : path(path), name(name), is_dir(isdir)
{
}

inline
Directory::Directory (void)
{
}

inline
Directory::Directory (std::string const& path)
{
    this->scan(path);
}

inline
FileLock::FileLock (void)
{
}

inline
FileLock::FileLock (std::string const& filename)
{
    this->acquire(filename);
}

inline
FileLock::~FileLock (void)
{
    this->release();
}

inline std::string const&
FileLock::get_reason (void) const
{
    return this->reason;
}

UTIL_FS_NAMESPACE_END
UTIL_NAMESPACE_END

#endif /* UTIL_FS_HEADER */
