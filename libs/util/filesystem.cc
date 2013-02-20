#include <iostream>
#include <fstream>
#include <cerrno> // errno
#include <cstring> // std::strerror
#include <cstdio> // std::rename

#if defined(_WIN32)
#   include <dirent.h>
#   include <direct.h>
#   include <io.h>
#   include <shlobj.h>
#   include <sys/stat.h>
#   include <sys/types.h>
#else // Linux, OSX, ...
#   include <dirent.h>
#   include <unistd.h>
#   include <sys/stat.h>
#   include <sys/types.h>
#   include <pwd.h>
#endif

#if defined(__APPLE__)
#   include <mach-o/dyld.h> // for _NSGetExecutablePath
#endif

#ifndef PATH_MAX
#   define PATH_MAX 2048
#endif

#include "exception.h"
#include "system.h"
#include "filesystem.h"

UTIL_NAMESPACE_BEGIN
UTIL_FS_NAMESPACE_BEGIN

// PATH_MAX might be long?
char home_path[PATH_MAX] = { 0 };

#ifdef _WIN32

bool
dir_exists(char const* pathname)
{
  struct _stat statbuf;
  if (::_stat(pathname, &statbuf) < 0)
  {
    return false;
  }

  if (!(statbuf.st_mode & _S_IFDIR))
    return false;

  return true;
}

/* ---------------------------------------------------------------- */

bool
file_exists(char const* pathname)
{
  struct _stat statbuf;
  if (::_stat(pathname, &statbuf) < 0)
  {
    return false;
  }

  if (!(statbuf.st_mode & _S_IFREG))
    return false;

  return true;
}

/* ---------------------------------------------------------------- */

// SHGetFolderPathA seems to expect non-wide chars
// http://msdn.microsoft.com/en-us/library/bb762181(VS.85).aspx

char*
get_default_home_path(void)
{
  if (*home_path != 0)
    return home_path;

  if (!SUCCEEDED(::SHGetFolderPathA(0, CSIDL_APPDATA, 0, 0, home_path)))
  {
    std::cout << "Warning: Couldn't determine home directory!" << std::endl;
    return 0;
  }

  return home_path;
}

/* ---------------------------------------------------------------- */

char*
get_cwd(char* buf, size_t size)
{
  return ::_getcwd(buf, size);
}

/* ---------------------------------------------------------------- */

bool
set_cwd (char const* pathname)
{
    return ::_chdir(pathname) >= 0;
}

/* ---------------------------------------------------------------- */

bool
mkdir(char const* pathname/*, mode_t mode*/)
{
  if (::_mkdir(pathname) < 0)
    return false;

  return true;
}

/* ---------------------------------------------------------------- */

bool
unlink(char const* pathname)
{
  return ::_unlink(pathname) >= 0;
}

/* ---------------------------------------------------------------- */
#else /* Not windows, assume POSIX standards. */
/* ---------------------------------------------------------------- */

bool
dir_exists(char const* pathname)
{
  struct stat statbuf;
  if (::stat(pathname, &statbuf) < 0)
  {
    return false;
  }

  if (!S_ISDIR(statbuf.st_mode))
    return false;

  return true;
}

/* ---------------------------------------------------------------- */

bool
file_exists(char const* pathname)
{
  struct stat statbuf;
  if (::stat(pathname, &statbuf) < 0)
  {
    return false;
  }

  if (!S_ISREG(statbuf.st_mode))
    return false;

  return true;
}

/* ---------------------------------------------------------------- */

char*
get_default_home_path(void)
{
  if (*home_path != 0)
    return home_path;

  uid_t user_id = ::geteuid();
  struct passwd* user_info = ::getpwuid(user_id);

  if (user_info == 0 || user_info->pw_dir == 0)
  {
    std::cout << "Warning: Couldn't determine home directory!" << std::endl;
    return 0;
  }

  std::strncpy(home_path, user_info->pw_dir, PATH_MAX);

  return home_path;
}

/* ---------------------------------------------------------------- */

char*
get_cwd(char* buf, size_t size)
{
  return ::getcwd(buf, size);
}

/* ---------------------------------------------------------------- */

bool
set_cwd (char const* pathname)
{
    return ::chdir(pathname) >= 0;
}

/* ---------------------------------------------------------------- */

bool
mkdir(char const* pathname/*, mode_t mode*/)
{
  if (::mkdir(pathname, S_IRWXU | S_IRGRP | S_IXGRP) < 0)
    return false;

  return true;
}

/* ---------------------------------------------------------------- */

bool
unlink(char const* pathname)
{
  return ::unlink(pathname) >= 0;
}

/* ---------------------------------------------------------------- */
#endif /* os check */
/* ---------------------------------------------------------------- */

// Availale for POSIX and Win32??
bool
rename (char const* from, char const* to)
{
    if (std::rename(from, to) < 0)
        return false;

    return true;
}

/* ---------------------------------------------------------------- */

std::string
get_cwd_string (void)
{
    std::size_t size = 1 << 8;
    while (true)
    {
        char* buf = new char[size];
        if (get_cwd(buf, size) == buf)
        {
            std::string ret(buf);
            delete[] buf;
            return ret;
        }
        delete[] buf;
        size = size << 1;
        if (size > (1 << 15))
            throw Exception("Error storing CWD");
    }
}

/* ---------------------------------------------------------------- */
// TODO Test on Win32 and OSX

std::string
get_binary_path (void)
{
    char path[PATH_MAX];
    std::fill(path, path + PATH_MAX, '\0');

#if defined(_WIN32)

#   ifdef UNICODE
#       error "Unicode defined but not supported"
#   endif

    TCHAR omgwtf[PATH_MAX];
    int n_chars = GetModuleFileName(NULL, omgwtf, PATH_MAX);
    std::copy(omgwtf, omgwtf + n_chars, path);

#elif defined(__APPLE__)

    // http://developer.apple.com/library/mac/#documentation/Darwin/Reference/ManPages/man3/dyld.3.html
    uint32_t pathmax = PATH_MAX;
    int success = _NSGetExecutablePath(path, &pathmax);
    int n_chars = 0;
    if (success < 0)
        n_chars = PATH_MAX; // Indicate error

#elif defined(__linux)

    int n_chars = ::readlink("/proc/self/exe", path, PATH_MAX);

#else
#   error "Cannot determine binary path: Unsupported OS"
#endif

    if (n_chars >= PATH_MAX)
        throw std::runtime_error("Buffer size too small!");

    return std::string(path);
}

/* ---------------------------------------------------------------- */
// FIXME windows path separator ("\") compatible version?
// FIXME handle relative paths

std::string
get_path_component (std::string const& path)
{
    if (path.empty())
        return get_cwd_string();

    std::size_t pos1 = path.find_first_of('/');
    std::size_t pos2 = path.find_last_of('/');

    if (pos1 == std::string::npos && pos2 == std::string::npos)
        return get_cwd_string();
    else if (pos1 == pos2 && pos1 == 0)
        return std::string("/");
    else if (pos1 == pos2 && pos1 != 0)
        return get_cwd_string() + "/" + path.substr(0, pos1);
    else if (pos1 == 0 && pos2 > 0)
        return path.substr(0, pos2);

    throw std::runtime_error("Unhandled path case");
}

/* ---------------------------------------------------------------- */
// FIXME Win32 compatible version

std::string
get_file_component (std::string const& path)
{
    if (path.empty() || path[path.size() - 1] == '/')
        return std::string();

    std::size_t pos = path.find_last_of('/');
    if (pos == std::string::npos)
        return path;

    return path.substr(pos + 1);
}

/* ---------------------------------------------------------------- */

std::string
replace_extension (std::string const& fn, std::string const& ext)
{
    std::size_t slashpos = fn.find_last_of('/');
    if (slashpos == std::string::npos)
        slashpos = 0;
    std::size_t dotpos = fn.find_last_of('.');
    if (dotpos == std::string::npos || dotpos < slashpos)
        return fn + "." + ext;

    std::string ret;
    ret.insert(ret.end(), fn.begin(), fn.begin() + dotpos);
    ret.push_back('.');
    ret.insert(ret.end(), ext.begin(), ext.end());
    return ret;
}

UTIL_FS_NAMESPACE_END
UTIL_NAMESPACE_END

/* ----------------------------------------------------------------
 * ----------------- File and Directory abstraction ---------------
 * ---------------------------------------------------------------- */

UTIL_NAMESPACE_BEGIN
UTIL_FS_NAMESPACE_BEGIN

std::string
File::get_absolute_name (void) const
{
#ifdef _WIN32
    return (!path.empty() && *path.rbegin() == '\\'
        ? path + name
        : path + "\\" + name);
#else
    return (!path.empty() && *path.rbegin() == '/'
        ? path + name
        : path + "/" + name);
#endif
}

/* ---------------------------------------------------------------- */

bool
File::operator< (File const& rhs) const
{
    if (this->is_dir && !rhs.is_dir)
        return true;
    else if (!this->is_dir && rhs.is_dir)
        return false;
    else if (this->path < rhs.path)
        return true;
    else if (rhs.path < this->path)
        return false;
    else
        return (this->name < rhs.name);
}

/* ---------------------------------------------------------------- */

void
Directory::scan (std::string const& path)
{
    this->clear();

    DIR *dp = ::opendir(path.c_str());
    if (dp == 0)
        throw Exception("Cannot open directory: ", std::strerror(errno));

    struct dirent *ep;
    while ((ep = ::readdir(dp)))
    {
        if (!std::strcmp(ep->d_name, "."))
            continue;
        if (!std::strcmp(ep->d_name, ".."))
            continue;
        this->push_back(File());
        this->back().path = path;
        this->back().name = ep->d_name;
#ifdef _WIN32
        std::string fname(this->back().path + "/" + this->back().name);
        this->back().is_dir = dir_exists(fname.c_str());
#else
        this->back().is_dir = (ep->d_type == DT_DIR);
#endif
    }
    ::closedir(dp);
}

/*
 * --------------------- File locking mechanism ----------------------
 */

bool
FileLock::acquire (std::string const& filename)
{
    /* Check if lock file exists. */
    this->lockfile = filename + ".lock";
    if (fs::file_exists(this->lockfile.c_str()))
    {
        this->reason = "File is locked";
        return false;
    }

    /* Finally create the lock file. */
    std::ofstream touch(this->lockfile.c_str());
    if (!touch.good())
    {
        this->reason = ::strerror(errno);
        return false;
    }
    touch.close();

    /* Return success, lock is created. */
    return true;
}

bool
FileLock::acquire_retry (std::string const& filename, int retries, int sleep)
{
    /* Try to acquire file lock for 'retry' times. */
    while (retries && !this->acquire(filename))
    {
        system::sleep(sleep);
        retries -= 1;

        /* Fail if all retries have been used. */
        if (!retries)
        {
            this->reason = "Previous lock persisted";
            return false;
        }
    }

    /* Return success, lock is created. */
    return true;
}

bool
FileLock::is_locked (std::string const& filename)
{
    std::string lockfname = filename + ".lock";
    return fs::file_exists(lockfname.c_str());
}

bool
FileLock::wait_lock (std::string const& filename, int retries, int sleep)
{
    while (retries && this->is_locked(filename))
    {
        system::sleep(sleep);
        retries -= 1;

        if (!retries)
        {
            this->reason = "Lock persisted";
            return false;
        }
    }

    return true;
}

bool
FileLock::release (void)
{
    if (this->lockfile.empty())
        return false;
    return fs::unlink(this->lockfile.c_str());
}

UTIL_FS_NAMESPACE_END
UTIL_NAMESPACE_END
