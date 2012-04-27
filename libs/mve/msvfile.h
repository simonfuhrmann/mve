/*
 * Microsoft Video Viewer MSV file reader.
 * Written by Simon Fuhrmann.
 */
#ifndef MVE_MSV_FILE_HEADER
#define MVE_MSV_FILE_HEADER

#include <fstream>
#include <string>

#include "defines.h"
#include "image.h"

MVE_NAMESPACE_BEGIN

/** Channels in the MSV files. */
enum MsvChannels
{
    MSV_CHANNEL_X, MSV_CHANNEL_Y, MSV_CHANNEL_Z, // World coords
    MSV_CHANNEL_R, MSV_CHANNEL_G, MSV_CHANNEL_B, // Color values
    MSV_CHANNEL_C, // Confidence
    MSV_CHANNEL_D, // Depth

    MSV_CHANNEL_AMOUNT // Keep last
};

/** Representation of the MSV file headers. */
struct MsvHeaders
{
    int version;
    int cbit;
    int left, right;
    int top, bottom;
    int channels;
    int bytesperpixel;
    int vispixfmt;

    MsvHeaders (void);
};

/* ---------------------------------------------------------------- */

/**
 * Class to read multichannel MSV files.
 *
 * // TODO More error checks in reader functions
 */
class MsvFile
{
  private:
    std::string filename;
    MsvHeaders headers;

  private:
    void read_headers_intern (std::ifstream& handle);

  public:
    MsvFile (void);
    MsvFile (std::string const& filename);

    /** Sets the filename. */
    void set_filename (std::string const& filename);

    /** Opens the input filename and reads headers only. */
    void read_headers (void);

    /** Prints a debug dump of the headers to the console. */
    void debug_print_headers (void);

    /** Opens the file, reads the headers and creates a color image. */
    ByteImage::Ptr load_rgb_image (void);

    /** Opens the file, reads the headers and creates a depth map. */
    FloatImage::Ptr load_depthmap (void);

    /** Opens file, reads headers and creates a confidence map. */
    FloatImage::Ptr load_confidence (void);

    /** Opens the file, reads the headers and creates float image. */
    FloatImage::Ptr load_all (void);
    
    /** Opens the file and reads a specific channel from file. */
    FloatImage::Ptr load_channel (std::size_t channel);

    /** Returns the headers. */
    MsvHeaders const& get_headers (void) const;

};

/* ------------------------------------------------------------ */

inline
MsvHeaders::MsvHeaders (void)
{
    this->version = 1;
    this->cbit = 4;
    this->left = 0;
    this->right = 0;
    this->top = 0;
    this->bottom = 0;
    this->channels = 8;
    this->bytesperpixel = 32;
    this->vispixfmt = 0x185;
}

inline
MsvFile::MsvFile (void)
{
}

inline
MsvFile::MsvFile (std::string const& filename)
{
    this->filename = filename;
}

inline void
MsvFile::set_filename (std::string const& filename)
{
    this->filename = filename;
}

inline MsvHeaders const&
MsvFile::get_headers (void) const
{
    return this->headers;
}

MVE_NAMESPACE_END

#endif /* MVE_MSV_FILE_HEADER */
