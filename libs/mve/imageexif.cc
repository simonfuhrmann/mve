/*
 * A rewritten version of the code found at:
 * http://code.google.com/p/easyexif/
 *
 * Copyright (c) 2011 Simon Fuhrmann
 * Copyright (c) 2010 Mayank Lahiri <mlahiri@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * -- Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * -- Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL THE FREEBSD PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <stdexcept>

#include "imageexif.h"

MVE_NAMESPACE_BEGIN
MVE_IMAGE_NAMESPACE_BEGIN

unsigned int
parse32 (unsigned char const* buf, bool intel)
{
    typedef unsigned int UINT;
    return intel
        ? (((UINT)buf[3]<<24) | ((UINT)buf[2]<<16) | ((UINT)buf[1]<<8) | buf[0])
        : (((UINT)buf[0]<<24) | ((UINT)buf[1]<<16) | ((UINT)buf[2]<<8) | buf[3]);
}

/* ---------------------------------------------------------------- */

unsigned short
parse16 (unsigned char const* buf, bool intel)
{
    typedef unsigned int UINT;
    return intel
        ? (((UINT)buf[1]<<8) | buf[0])
        : (((UINT)buf[0]<<8) | buf[1]);
}

/* ---------------------------------------------------------------- */

unsigned char
parse8 (unsigned char const* buf)
{
    return buf[0];
}

/* ---------------------------------------------------------------- */

void
copyEXIFString (std::string& dest, unsigned char const* buf, unsigned int len)
{
    dest.clear();
    if (len == 0)
        return;
    dest.append(reinterpret_cast<char const*>(buf), len - 1);
}

/* ---------------------------------------------------------------- */

float
parseEXIFrational(unsigned char const* buf)
{
    float numerator = (float)*((unsigned int*)buf);
    float denominator = (float)*(((unsigned int*)buf)+1);
    if (denominator < 1e-20)
        return 0.0f;
    return numerator / denominator;
}

/* ---------------------------------------------------------------- */

inline bool
ifd_is_offset (unsigned short type, unsigned int count)
{
    /*
     * TIFF Types with bit count:
     * 1: BYTE 8, 2: ASCII 8, 3: SHORT 16, 4: LONG 32, 5: RATIONAL 64
     * 6: SBYTE 8, 7: UNDEF 8, 8: SSHORT 16, 9: SLONG 32, 10: SRATIONAL 64,
     * 11: FLOAT 32, 12: DOUBLE 64
     */

    return (type == 1 && count > 4)
        || (type == 2 && count > 4)
        || (type == 3 && count > 2)
        || (type == 4 && count > 1)
        || (type == 5 && count > 0)
        || (type == 6 && count > 4)
        || (type == 7 && count > 4)
        || (type == 8 && count > 2)
        || (type == 9 && count > 1)
        || (type == 10 && count > 0)
        || (type == 11 && count > 1)
        || (type == 12 && count > 0);
}

/* ---------------------------------------------------------------- */

ExifInfo
exif_extract (char const* data, unsigned int len, bool is_jpeg)
{
    unsigned char const* buf = reinterpret_cast<unsigned char const*>(data);
    bool alignIntel = true; // byte alignment
    unsigned offs = 0; // current offset into buffer

    // Prepare return structure
    ExifInfo result;
    result.iso_speed = 0;
    result.focal_length = 0.0f;
    result.focal_length_35mm = 0.0f;
    result.f_number = 0.0f;
    result.exposure_time = 0.0f;

    // Scan for EXIF header and do a sanity check
    if (is_jpeg)
    {
        for (offs = 0; offs < len-1; offs++)
            if(buf[offs] == 0xFF && buf[offs+1] == 0xE1)
                break;
        if (offs == len-1)
            throw std::invalid_argument("Cannot find EXIF marker!");
        offs += 4;
    }

    // byte 0-3 header, 6-7 endian, 10-13 IFD offset
    if (offs + 14 > len)
        throw std::invalid_argument("EXIF data corrupt (header)");

    // check EXIF header
    if (buf[offs] != 0x45 || buf[offs+1] != 0x78 || buf[offs+2] != 0x69)
        throw std::invalid_argument("Cannot find EXIF header");

    // Get byte alignment (Motorola or Intel)
    offs += 6;
    if (buf[offs] == 0x49 && buf[offs+1] == 0x49)
        alignIntel = true;
    else if (buf[offs] == 0x4d && buf[offs+1] == 0x4d)
        alignIntel = false;
    else
        throw std::invalid_argument("Cannot find EXIF byte alignment");

    // Get offset into first IFD
    offs += 4;
    unsigned int x = parse32(buf+offs, alignIntel);
    if (offs + x >= len)
        throw std::invalid_argument("EXIF data corrupt (IFD offset)");

    /*
     * Each IFD entry consists of 12 bytes. The first two bytes identifies
     * the tag type (as in Tagged Image File Format). The next two bytes are
     * the field type (byte, ASCII, short int, long int, ...). The next four
     * bytes indicate the number of values. The last four bytes is either
     * the value itself or an offset to the values.
     */

    // Jump to the first IFD, scan tags there.
    offs += x-4;
    int nentries = parse16(buf+offs, alignIntel);
    offs += 2;
    unsigned int ifdOffset = offs-10;
    unsigned int exifSubIFD = 0;

    if (offs + 12 * nentries > len)
        throw std::invalid_argument("EXIF data corrupt (IFD table)");

    for (int j = 0; j < nentries; j++)
    {
        unsigned short tag = parse16(buf+offs, alignIntel);
        unsigned short type = parse16(buf+offs+2, alignIntel);
        unsigned int ncomp = parse32(buf+offs+4, alignIntel);
        unsigned int coffs = parse32(buf+offs+8, alignIntel);
        unsigned int buf_off = ifdOffset + coffs;

        if (ifd_is_offset(type, ncomp) && buf_off + ncomp > len)
            throw std::invalid_argument("EXIF data corrupt (IFD entry)");

        switch (tag)
        {
            case 0x8769: // EXIF subIFD offset
                exifSubIFD = ifdOffset + coffs;
                break;

            case 0x10F: // Digicam manufacturer
                copyEXIFString(result.camera_maker, buf + buf_off, ncomp);
                break;

            case 0x110: // Digicam model
                copyEXIFString(result.camera_model, buf + buf_off, ncomp);
                break;

            case 0x132: // EXIF/TIFF date/time of image
                copyEXIFString(result.date_modified, buf + buf_off, ncomp);
                break;

            case 0x10E: // image description
                copyEXIFString(result.description, buf + buf_off, ncomp);
                break;
        }
        offs += 12;
    }
    if (!exifSubIFD)
        return result;

    // At the EXIF SubIFD, read the rest of the EXIF tags
    offs = exifSubIFD;
    nentries = parse16(buf+offs, alignIntel);
    offs += 2;

    if (offs + 12 * nentries > len)
        throw std::invalid_argument("EXIF data corrupt (SubIFD table)");

    for (int j = 0; j < nentries; j++)
    {
        unsigned short tag = parse16(buf+offs, alignIntel);
        unsigned short type = parse16(buf+offs+2, alignIntel);
        unsigned int ncomp = parse32(buf+offs+4, alignIntel);
        unsigned int coffs = parse32(buf+offs+8, alignIntel);
        unsigned int buf_off = ifdOffset + coffs;
        //std::cout << "tag: " << tag << ", type: " << type
        //    << ", ncomp: " << ncomp << ", coffs: "
        //    << coffs << std::endl;

        if (ifd_is_offset(type, ncomp) && buf_off + ncomp > len)
            throw std::invalid_argument("EXIF data corrupt (SubIFD entry)");

        switch (tag)
        {
            case 0x9003: // original image date/time string
                copyEXIFString(result.date_original, buf + buf_off, ncomp);
                break;

            case 0x8827: // ISO speed ratings
                result.iso_speed = parse16(buf+offs+8, alignIntel);
                break;

            case 0x920A: // Focal length in mm
                result.focal_length = parseEXIFrational(buf + buf_off);
                break;

            case 0xA405: // Focal length in 35 mm
                result.focal_length_35mm = (float)parse16(buf+offs+8, alignIntel);
                break;

            case 0x829D: // F-stop
                result.f_number = parseEXIFrational(buf + buf_off);
                break;

            case 0x829A: // Exposure time
                result.exposure_time = parseEXIFrational(buf + buf_off);
                break;
        }
        offs += 12;
    }

    return result;
}

/* ---------------------------------------------------------------- */

void
exif_debug_print (std::ostream& stream, ExifInfo const& exif)
{
    stream
        << "Camera manufacturer: " << exif.camera_maker << std::endl
        << "Camera model: " << exif.camera_model << std::endl
        << "Description: " << exif.description << std::endl
        << "Date (modified): " << exif.date_modified << std::endl
        << "Date (original): " << exif.date_original << std::endl
        << "Focal length: " << exif.focal_length << "mm" << std::endl
        << "Focal length (35mm): " << exif.focal_length_35mm << "mm" << std::endl
        << "F-Number: f/" << exif.f_number << std::endl
        << "Exposure time: " << exif.exposure_time << "sec" << std::endl
        << "ISO speed: " << exif.iso_speed << std::endl;
}

MVE_IMAGE_NAMESPACE_END
MVE_NAMESPACE_END
