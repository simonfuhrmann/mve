/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <cctype>

#include "camera_database.h"

SFM_NAMESPACE_BEGIN

CameraDatabase* CameraDatabase::instance = NULL;

namespace
{
    /**
     * Given a string, compute a simplified version that only contains upper
     * case alpha-numeric characters. All other characters are replaced with
     * spaces. Two or more subsequent spaces are replaced with a single space.
     *
     * Example: Asahi Optical Co.,Ltd.  PENTAX Optio330RS
     * Becomes: ASAHI OPTICAL CO LTD PENTAX OPTIO330RS
     */
    std::string
    simplify_string (std::string const& str)
    {
        std::string ret;

        bool was_alpha_numeric = true;
        for (std::size_t i = 0; i < str.size(); ++i)
        {
            if (std::isalnum(str[i]))
            {
                if (!was_alpha_numeric)
                    ret.append(1, ' ');
                ret.append(1, std::toupper(str[i]));
                was_alpha_numeric = true;
            }
            else
                was_alpha_numeric = false;
        }

        return ret;
    }
}

void
CameraDatabase::add (std::string const& maker, std::string const& model,
    float sensor_width_mm, float sensor_height_mm,
    int sensor_width_px, int sensor_height_px)
{
    this->data.push_back(CameraModel());
    CameraModel& cam = this->data.back();
    cam.maker = simplify_string(maker);
    cam.model = simplify_string(model);
    cam.sensor_width_mm = sensor_width_mm;
    cam.sensor_height_mm = sensor_height_mm;
    cam.sensor_width_px = sensor_width_px;
    cam.sensor_height_px = sensor_height_px;
}

CameraModel const*
CameraDatabase::lookup (std::string const& maker,
    std::string const& model) const
{
    std::string const s_maker = simplify_string(maker);
    std::string const s_model = simplify_string(model);
    for (std::size_t i = 0; i < this->data.size(); ++i)
        if (this->data[i].maker == s_maker && this->data[i].model == s_model)
            return &this->data[i];
    return NULL;
}

CameraDatabase::CameraDatabase (void)
{
    /*
     * The following code is generated from a camera database.
     * Do not change it by hand as changes will be overwritten.
     *
     * TODO: Add all cameras in the known universe.
     */

    /* Cameras from Canon. */
    this->add("Canon", "Canon IXUS 1100 HS", 6.17f, 4.55f, 4000, 3000);
    this->add("Canon", "Canon EOS 1000D", 22.2f, 14.8f, 3888, 2592);
    this->add("Canon", "Canon EOS DIGITAL REBEL XS", 22.2f, 14.8f, 3888, 2592);
    this->add("Canon", "Canon EOS REBEL SL1", 22.3f, 14.9f, 5184, 3456);
    this->add("Canon", "Canon EOS 10D", 22.7f, 15.1f, 3072, 2048);
    this->add("Canon", "Canon EOS 20D", 22.5f, 15.0f, 3504, 2336);
    this->add("Canon", "Canon EOS 300D DIGITAL", 22.7f, 15.1f, 3072, 2048);
    this->add("Canon", "Canon EOS 30D", 22.5f, 15.0f, 3504, 2336);
    this->add("Canon", "Canon EOS 350D DIGITAL", 22.2f, 14.8f, 3456, 2304);
    this->add("Canon", "Canon EOS 400D DIGITAL", 22.2f, 14.8f, 3888, 2592);
    this->add("Canon", "Canon EOS Kiss Digital X", 22.2f, 14.8f, 3888, 2592);
    this->add("Canon", "Canon EOS DIGITAL REBEL XTi", 22.2f, 14.8f, 3888, 2592);
    this->add("Canon", "Canon EOS 40D", 22.2f, 14.8f, 3888, 2592);
    this->add("Canon", "Canon EOS 450D", 22.2f, 14.8f, 4272, 2848);
    this->add("Canon", "Canon EOS 500D", 22.3f, 14.9f, 4752, 3168);
    this->add("Canon", "Canon EOS 50D", 22.3f, 14.9f, 4752, 3168);
    this->add("Canon", "Canon EOS 550D", 22.3f, 14.9f, 5184, 3456);
    this->add("Canon", "Canon EOS 5D", 36.0f, 24.0f, 4368, 2912);
    this->add("Canon", "Canon EOS 5D Mark II", 36.0f, 24.0f, 5616, 3744);
    this->add("Canon", "Canon EOS 5D Mark III", 36.0f, 24.0f, 5760, 3840);
    this->add("Canon", "Canon EOS 600D", 22.3f, 14.9f, 5184, 3456);
    this->add("Canon", "Canon EOS 60D", 22.3f, 14.9f, 5184, 3456);
    this->add("Canon", "Canon EOS REBEL T4i", 22.3f, 14.9f, 5184, 3456);
    this->add("Canon", "Canon EOS 6D", 36.0f, 24.0f, 5472, 3648);
    this->add("Canon", "Canon EOS 700D", 22.3f, 14.9f, 5184, 3456);
    this->add("Canon", "Canon EOS REBEL T5i", 22.3f, 14.9f, 5184, 3456);
    this->add("Canon", "Canon EOS 70D", 22.5f, 15.0f, 5472, 3648);
    this->add("Canon", "Canon EOS 7D", 22.3f, 14.9f, 5184, 3456);
    this->add("Canon", "Canon EOS M", 22.3f, 14.9f, 5184, 3456);
    this->add("Canon", "Canon EOS-1D", 28.7f, 19.1f, 2464, 1648);
    this->add("Canon", "Canon EOS-1D Mark II", 28.7f, 19.1f, 3504, 2336);
    this->add("Canon", "Canon EOS-1D Mark IV", 27.9f, 18.6f, 4896, 3264);
    this->add("Canon", "Canon EOS-1Ds Mark II", 35.8f, 23.8f, 4064, 2704);
    this->add("Canon", "Canon EOS-1Ds Mark II", 36.0f, 24.0f, 4992, 3328);
    this->add("Canon", "Canon EOS-1Ds Mark III", 36.0f, 24.0f, 5616, 3744);
    this->add("Canon", "Canon PowerShot A20", 5.312f, 3.984f, 1600, 1200);
    this->add("Canon", "Canon PowerShot A300", 5.312f, 3.984f, 2048, 1536);
    this->add("Canon", "Canon PowerShot A40", 5.312f, 3.984f, 1600, 1200);
    this->add("Canon", "Canon PowerShot A510", 5.744f, 4.308f, 2048, 1536);
    this->add("Canon", "Canon PowerShot A520", 5.744f, 4.308f, 2272, 1704);
    this->add("Canon", "Canon PowerShot A570 IS", 5.744f, 4.308f, 3072, 2304);
    this->add("Canon", "Canon PowerShot A620", 7.144f, 5.358f, 3072, 2304);
    this->add("Canon", "Canon PowerShot A640", 7.144f, 5.358f, 3648, 2736);
    this->add("Canon", "Canon PowerShot A70", 5.312f, 3.984f, 2048, 1536);
    this->add("Canon", "Canon PowerShot A700", 5.744f, 4.308f, 2816, 2112);
    this->add("Canon", "Canon PowerShot A710 IS", 5.744f, 4.308f, 3072, 2304);
    this->add("Canon", "Canon PowerShot A720 IS", 5.744f, 4.308f, 3264, 2448);
    this->add("Canon", "Canon PowerShot A95", 7.144f, 5.358f, 2592, 1944);
    this->add("Canon", "Canon PowerShot D10", 6.17f, 4.55f, 4000, 3000);
    this->add("Canon", "Canon PowerShot D20", 6.17f, 4.55f, 4000, 3000);
    this->add("Canon", "Canon PowerShot G1", 7.144f, 5.358f, 2048, 1536);
    this->add("Canon", "Canon PowerShot G1 X", 18.7f, 14.0f, 4352, 3264);
    this->add("Canon", "Canon PowerShot G10", 7.44f, 5.58f, 4416, 3312);
    this->add("Canon", "Canon PowerShot G11", 7.44f, 5.58f, 3648, 2736);
    this->add("Canon", "Canon PowerShot G12", 7.44f, 5.58f, 3648, 2736);
    this->add("Canon", "Canon PowerShot G15", 7.44f, 5.58f, 4000, 3000);
    this->add("Canon", "Canon PowerShot G16", 7.44f, 5.58f, 4000, 3000);
    this->add("Canon", "Canon PowerShot G2", 7.144f, 5.358f, 2272, 1704);
    this->add("Canon", "Canon PowerShot G3", 7.144f, 5.358f, 2272, 1704);
    this->add("Canon", "Canon PowerShot G5", 7.144f, 5.358f, 2592, 1944);
    this->add("Canon", "Canon PowerShot G6", 7.144f, 5.358f, 3072, 2304);
    this->add("Canon", "Canon PowerShot G7", 7.144f, 5.358f, 3648, 2736);
    this->add("Canon", "Canon PowerShot G9", 7.44f, 5.58f, 4000, 3000);
    this->add("Canon", "Canon PowerShot Pro1", 8.8f, 6.6f, 3264, 2448);
    this->add("Canon", "Canon PowerShot Pro90 IS", 7.144f, 5.358f, 1856, 1392);
    this->add("Canon", "Canon PowerShot S1 IS", 5.312f, 3.984f, 2048, 1536);
    this->add("Canon", "Canon PowerShot S100", 7.44f, 5.58f, 4000, 3000);
    this->add("Canon", "Canon PowerShot S110", 7.44f, 5.58f, 4000, 3000);
    this->add("Canon", "Canon DIGITAL IXUS", 5.312f, 3.984f, 1600, 1200);
    this->add("Canon", "Canon DIGITAL IXUS v", 5.312f, 3.984f, 1600, 1200);
    this->add("Canon", "Canon PowerShot S120", 7.44f, 5.58f, 4000, 3000);
    this->add("Canon", "Canon PowerShot S2 IS", 5.744f, 4.308f, 2592, 1944);
    this->add("Canon", "Canon PowerShot S20", 7.144f, 5.358f, 2048, 1536);
    this->add("Canon", "Canon PowerShot S3 IS", 5.744f, 4.308f, 2816, 2112);
    this->add("Canon", "Canon DIGITAL IXUS 300", 5.312f, 3.984f, 1600, 1200);
    this->add("Canon", "Canon DIGITAL IXUS 330", 5.312f, 3.984f, 1600, 1200);
    this->add("Canon", "Canon PowerShot S40", 7.144f, 5.358f, 2272, 1704);
    this->add("Canon", "Canon DIGITAL IXUS 400", 7.144f, 5.358f, 2272, 1704);
    this->add("Canon", "Canon PowerShot S45", 7.144f, 5.358f, 2272, 1704);
    this->add("Canon", "Canon PowerShot S5 IS", 5.744f, 4.308f, 3264, 2448);
    this->add("Canon", "Canon PowerShot S50", 7.144f, 5.358f, 2592, 1944);
    this->add("Canon", "Canon DIGITAL IXUS 500", 7.144f, 5.358f, 2592, 1944);
    this->add("Canon", "Canon PowerShot S60", 7.144f, 5.358f, 2592, 1944);
    this->add("Canon", "Canon PowerShot S70", 7.144f, 5.358f, 3072, 2304);
    this->add("Canon", "Canon PowerShot S80", 7.144f, 5.358f, 3264, 2448);
    this->add("Canon", "Canon PowerShot S90", 7.44f, 5.58f, 3648, 2736);
    this->add("Canon", "Canon PowerShot S95", 7.44f, 5.58f, 3648, 2736);
    this->add("Canon", "Canon DIGITAL IXUS II", 5.312f, 3.984f, 2048, 1536);
    this->add("Canon", "Canon DIGITAL IXUS 80 IS", 5.744f, 4.308f, 3264, 2448);
    this->add("Canon", "Canon DIGITAL IXUS 40", 5.744f, 4.308f, 2272, 1704);
    this->add("Canon", "Canon IXY DIGITAL 50", 5.744f, 4.308f, 2272, 1704);
    this->add("Canon", "Canon DIGITAL IXUS 50", 5.744f, 4.308f, 2592, 1944);
    this->add("Canon", "Canon IXY 30S", 6.17f, 4.55f, 3648, 2736);
    this->add("Canon", "Canon DIGITAL IXUS 55", 5.744f, 4.308f, 2592, 1944);
    this->add("Canon", "Canon IXUS 1000HS", 6.17f, 4.55f, 3648, 2736);
    this->add("Canon", "Canon DIGITAL IXUS 700", 7.144f, 5.358f, 3072, 2304);
    this->add("Canon", "Canon DIGITAL IXUS 750", 7.144f, 5.358f, 3072, 2304);
    this->add("Canon", "Canon DIGITAL IXUS 800 IS", 5.744f, 4.308f, 2816, 2112);
    this->add("Canon", "Canon DIGITAL IXUS 850 IS", 5.744f, 4.308f, 3072, 2304);
    this->add("Canon", "Canon DIGITAL IXUS 950 IS", 5.744f, 4.308f, 3264, 2448);
    this->add("Canon", "Canon DIGITAL IXUS 860 IS", 5.744f, 4.308f, 3264, 2448);
    this->add("Canon", "Canon DIGITAL IXUS 900Ti", 7.144f, 5.358f, 3648, 2736);
    this->add("Canon", "Canon DIGITAL IXUS 990 IS", 6.17f, 4.55f, 4000, 3000);
    this->add("Canon", "Canon PowerShot SX1 IS", 6.17f, 4.55f, 3648, 2736);
    this->add("Canon", "Canon PowerShot SX100 IS", 5.744f, 4.308f, 3264, 2448);
    this->add("Canon", "Canon PowerShot SX150 IS", 6.17f, 4.55f, 4320, 3240);
    this->add("Canon", "Canon PowerShot SX20 IS", 6.17f, 4.55f, 4000, 3000);
    this->add("Canon", "Canon PowerShot SX200 IS", 6.17f, 4.55f, 4000, 3000);
    this->add("Canon", "Canon PowerShot SX210 IS", 6.17f, 4.55f, 4320, 3240);
    this->add("Canon", "Canon PowerShot SX230 HS", 6.17f, 4.55f, 4000, 3000);
    this->add("Canon", "Canon PowerShot SX260 HS", 6.17f, 4.55f, 4000, 3000);
    this->add("Canon", "Canon PowerShot SX50 HS", 6.17f, 4.55f, 4000, 3000);
    this->add("Canon", "Canon PowerShot TX1", 5.744f, 4.308f, 3072, 2304);

    /* Cameras from Casio. */
    this->add("CASIO COMPUTER CO.,LTD.", "EX-ZR100", 6.17f, 4.55f, 4000, 3000);
    this->add("CASIO COMPUTER CO.,LTD.", "EX-FH100", 6.17f, 4.55f, 3648, 2736);
    this->add("CASIO COMPUTER CO.,LTD.", "EX-FH25", 6.17f, 4.55f, 3648, 2736);
    this->add("CASIO COMPUTER CO.,LTD.", "EX-V7", 5.744f, 4.308f, 3072, 2304);
    this->add("CASIO COMPUTER CO.,LTD.", "EX-Z1000", 7.144f, 5.358f, 3648, 2736);
    this->add("CASIO COMPUTER CO.,LTD.", "EX-Z850", 7.144f, 5.358f, 3264, 2448);
    this->add("CASIO COMPUTER CO.,LTD", "EX-Z750", 7.144f, 5.358f, 3072, 2304);
    this->add("CASIO COMPUTER CO.,LTD", "EX-P700", 7.144f, 5.358f, 3072, 2304);
    this->add("CASIO COMPUTER CO.,LTD", "EX-Z3", 5.744f, 4.308f, 2048, 1536);
    this->add("CASIO", "QV-4000", 7.144f, 5.358f, 2240, 1680);
    this->add("CASIO", "QV-3000EX", 7.144f, 5.358f, 2048, 1536);

    /* Cameras from Contax. */

    /* Cameras from Epson. */
    this->add("SEIKO EPSON CORP.", "PhotoPC 3000Z", 7.144f, 5.358f, 2048, 1536);

    /* Cameras from Fujifilm. */
    this->add("FUJIFILM", "X-T1", 23.6f, 15.6f, 4896, 3264);
    this->add("FUJIFILM", "X-E2", 23.6f, 15.6f, 4896, 3264);
    this->add("FUJIFILM", "XQ1", 8.8f, 6.6f, 4000, 3000);
    this->add("FUJIFILM", "X-A1", 23.6f, 15.6f, 4896, 3264);
    this->add("FUJIFILM", "X-M1", 23.6f, 15.6f, 4896, 3264);
    this->add("FUJIFILM", "X100S", 23.6f, 15.8f, 4896, 3264);
    this->add("FUJIFILM", "X20", 8.8f, 6.6f, 4000, 3000);
    this->add("FUJIFILM", "X-E1", 23.6f, 15.6f, 4896, 3264);
    this->add("FUJIFILM", "X-Pro1", 23.6f, 15.6f, 4896, 3264);
    this->add("FUJIFILM", "X10", 8.8f, 6.6f, 4000, 3000);
    this->add("FUJIFILM", "FinePix XP30", 6.17f, 4.55f, 4320, 3240);
    this->add("FUJIFILM", "FinePix F550EXR", 6.4f, 4.8f, 4608, 3456);
    this->add("FUJIFILM", "FinePix T300", 6.17f, 4.55f, 4288, 3216);
    this->add("FUJIFILM", "FinePix X100", 23.6f, 15.8f, 4288, 2848);
    this->add("FUJIFILM", "FinePix JZ500", 6.17f, 4.55f, 4320, 3240);
    this->add("FUJIFILM", "FinePix HS10 HS11", 6.17f, 4.55f, 3648, 2736);
    this->add("FUJIFILM", "FinePix F80EXR", 6.4f, 4.8f, 4000, 3000);
    this->add("FUJIFILM", "FinePix S2500HD", 6.17f, 4.55f, 4000, 3000);
    this->add("FUJIFILM", "FinePix F70EXR", 6.4f, 4.8f, 3616, 2712);
    this->add("FUJIFILM", "FinePix Z33WP", 6.17f, 4.55f, 3648, 2736);
    this->add("FUJIFILM", "FinePix F200EXR", 8.0f, 6.0f, 4000, 3000);
    this->add("FUJIFILM", "FinePix S5Pro", 23.0f, 15.5f, 4256, 2848);
    this->add("FUJIFILM", "FinePix S100FS", 8.8f, 6.6f, 3840, 2880);
    this->add("FUJIFILM", "FinePix F50fd", 8.0f, 6.0f, 4000, 3000);
    this->add("FUJIFILM", "FinePix S8000fd", 5.76f, 4.32f, 3264, 2448);
    this->add("FUJIFILM", "FinePix F31fd", 7.44f, 5.58f, 2848, 2136);
    this->add("FUJIFILM", "FinePix S6500fd", 7.44f, 5.58f, 2848, 2136);
    this->add("FUJIFILM", "FinePix F30", 7.44f, 5.58f, 2848, 2136);
    this->add("FUJIFILM", "FinePix S9500", 8.0f, 6.0f, 3488, 2616);
    this->add("FUJIFILM", "FinePix F10", 7.44f, 5.58f, 2848, 2136);
    this->add("FUJIFILM", "FinePix E550", 7.44f, 5.58f, 4048, 3040);
    this->add("FUJIFILM", "FinePix F810", 7.44f, 5.58f, 4048, 3040);
    this->add("FUJIFILM", "FinePix S5500", 5.312f, 3.984f, 2272, 1704);
    this->add("FUJIFILM", "FinePix S3Pro", 23.0f, 15.5f, 4256, 2848);
    this->add("FUJIFILM", "FinePix S7000", 7.44f, 5.58f, 4048, 3040);
    this->add("FUJIFILM", "FinePix S5000", 5.312f, 3.984f, 2816, 2120);
    this->add("FUJIFILM", "FinePix F700", 7.44f, 5.58f, 2832, 2128);
    this->add("FUJIFILM", "FinePix F601 ZOOM", 7.44f, 5.58f, 2832, 2128);
    this->add("FUJIFILM", "FinePix S602 ZOOM", 7.44f, 5.58f, 2832, 2128);
    this->add("FUJIFILM", "FinePixS2Pro", 23.0f, 15.5f, 4256, 2848);
    this->add("FUJIFILM", "FinePix6900ZOOM", 7.44f, 5.58f, 2832, 2128);
    this->add("FUJIFILM", "FinePix6800 ZOOM", 7.44f, 5.58f, 2832, 2128);
    this->add("FUJIFILM", "FinePix4900ZOOM", 7.44f, 5.58f, 2400, 1800);
    this->add("FUJIFILM", "FinePix40i", 7.44f, 5.58f, 2400, 1800);
    this->add("FUJIFILM", "FinePix4700 ZOOM", 7.44f, 5.58f, 2400, 1800);
    this->add("FUJIFILM", "FinePixS1Pro", 23.0f, 15.5f, 3040, 2016);
    this->add("SONY", "SLT-A55V", 6.4f, 4.8f, 1800, 1200);

    /* Cameras from HP. */
    this->add("Hewlett-Packard", "HP PhotoSmart R707 (V01.00)", 7.144f, 5.358f, 2608, 1952);
    this->add("Hewlett-Packard", "HP PhotoSmart C935 (V03.46)", 7.144f, 5.358f, 2608, 1952);
    this->add("Hewlett-Packard", "HP PhotoSmart C850 (V05.26)", 7.144f, 5.358f, 2272, 1712);
    this->add("Hewlett-Packard", "HP PhotoSmart C812 (V09.33)", 7.144f, 5.358f, 2272, 1712);

    /* Cameras from Kodak. */
    this->add("EASTMAN KODAK COMPANY", "KODAK EasyShare Z981 Digital Camera", 6.08f, 4.56f, 4288, 3216);
    this->add("EASTMAN KODAK COMPANY", "KODAK EASYSHARE Z950 DIGITAL CAMERA", 6.08f, 4.56f, 4000, 3000);
    this->add("EASTMAN KODAK COMPANY", "KODAK C875 ZOOM DIGITAL CAMERA", 7.144f, 5.358f, 3264, 2448);
    this->add("EASTMAN KODAK COMPANY", "KODAK Z612 ZOOM DIGITAL CAMERA", 5.744f, 4.308f, 2848, 2144);
    this->add("EASTMAN KODAK COMPANY", "KODAK Z650 ZOOM DIGITAL CAMERA", 5.744f, 4.308f, 2848, 2144);
    this->add("EASTMAN KODAK COMPANY", "KODAK P850 ZOOM DIGITAL CAMERA", 5.744f, 4.308f, 2592, 1994);
    this->add("EASTMAN KODAK COMPANY", "KODAK P880 ZOOM DIGITAL CAMERA", 7.144f, 5.358f, 3264, 2448);
    this->add("EASTMAN KODAK COMPANY", "KODAK Z740 ZOOM DIGITAL CAMERA", 5.744f, 4.308f, 2576, 1932);
    this->add("EASTMAN KODAK COMPANY", "KODAK DX7590 ZOOM DIGITAL CAMERA", 5.744f, 4.308f, 2576, 1932);
    this->add("Kodak", "Kodak DCS Pro SLR/c", 36.0f, 24.0f, 4500, 3000);
    this->add("Kodak", "DCS Pro SLR/n", 36.0f, 24.0f, 4500, 3000);
    this->add("Canon", "Canon PowerShot Pro1", 7.32f, 5.49f, 1760, 1168);

    /* Cameras from Konica Minolta. */
    this->add("KONICA MINOLTA", "MAXXUM 7D", 23.5f, 15.7f, 3008, 2000);
    this->add("KONICA MINOLTA", "DiMAGE Z5", 5.744f, 4.308f, 2560, 1920);
    this->add("KONICA MINOLTA", "DiMAGE A200", 8.8f, 6.6f, 3264, 2448);
    this->add("Konica Minolta Camera, Inc.", "DiMAGE Z2", 5.744f, 4.308f, 2272, 1704);
    this->add("Konica Minolta Camera, Inc.", "DiMAGE A2", 8.8f, 6.6f, 3264, 2448);
    this->add("Minolta Co., Ltd.", "DiMAGE A1", 8.8f, 6.6f, 2560, 1920);
    this->add("Minolta Co., Ltd.", "DiMAGE 7Hi", 8.8f, 6.6f, 2560, 1920);
    this->add("Minolta Co., Ltd.", "DiMAGE F100", 7.144f, 5.358f, 2272, 1704);
    this->add("Minolta Co., Ltd.", "DiMAGE 7i", 8.8f, 6.6f, 2560, 1920);
    this->add("Minolta Co., Ltd.", "DiMAGE S404", 7.144f, 5.358f, 2272, 1704);
    this->add("MINOLTA CO.,LTD", "DiMAGE X", 5.312f, 3.984f, 1600, 1200);
    this->add("Minolta Co., Ltd.", "DiMAGE S304", 7.144f, 5.358f, 2048, 1536);
    this->add("Minolta Co., Ltd.", "DiMAGE 5", 7.144f, 5.358f, 2048, 1536);
    this->add("Minolta Co., Ltd.", "DiMAGE 7", 8.8f, 6.6f, 2560, 1920);
    this->add("OLYMPUS IMAGING CORP.", "E-PL5", 17.3f, 13.0f, 4608, 3456);
    this->add("OLYMPUS IMAGING CORP.", "E-PL7", 17.3f, 13.0f, 4608, 3456);

    /* Cameras from Kyocera. */
    this->add("KYOCERA", "FC-S3", 7.144f, 5.358f, 2048, 1536);

    /* Cameras from Leica. */
    this->add("Leica Camera AG", "M Monochrom", 36.0f, 24.0f, 5212, 3472);
    this->add("LEICA", "V-LUX 3", 6.17f, 4.55f, 4000, 3000);
    this->add("Leica Camera AG", "M9 Digital Camera", 36.0f, 24.0f, 5212, 3472);
    this->add("Leica Camera AG", "M9 Digital Camera", 36.0f, 24.0f, 5212, 3472);
    this->add("LEICA CAMERA AG", "LEICA X1", 23.6f, 15.8f, 4272, 2856);
    this->add("Leica Camera AG", "S2", 45.0f, 30.0f, 7500, 5000);
    this->add("Leica Camera AG", "M8 Digital Camera", 27.0f, 18.0f, 3936, 2630);
    this->add("LEICA", "DIGILUX 2", 8.8f, 6.6f, 2560, 1920);

    /* Cameras from Nikon. */
    this->add("NIKON CORPORATION", "NIKON Df", 36.0f, 23.9f, 4928, 3280);
    this->add("NIKON CORPORATION", "NIKON D5300", 23.5f, 15.6f, 6000, 4000);
    this->add("NIKON CORPORATION", "NIKON D610", 35.9f, 24.0f, 6016, 4016);
    this->add("NIKON CORPORATION", "NIKON 1 AW1", 13.2f, 8.8f, 4608, 3072);
    this->add("NIKON", "COOLPIX P7800", 7.44f, 5.58f, 4000, 3000);
    this->add("NIKON CORPORATION", "COOLPIX A", 23.6f, 15.7f, 4928, 3264);
    this->add("NIKON CORPORATION", "NIKON D7100", 23.5f, 15.6f, 6000, 4000);
    this->add("NIKON", "COOLPIX AW110", 6.17f, 4.55f, 4608, 3456);
    this->add("NIKON CORPORATION", "NIKON 1 S1", 13.2f, 8.8f, 3872, 2592);
    this->add("NIKON CORPORATION", "NIKON 1 J3", 13.2f, 8.8f, 4608, 3072);
    this->add("NIKON CORPORATION", "NIKON D5200", 23.5f, 15.6f, 6000, 4000);
    this->add("NIKON CORPORATION", "NIKON 1 V2", 13.2f, 8.8f, 4608, 3072);
    this->add("NIKON CORPORATION", "NIKON D600", 35.9f, 24.0f, 6016, 4016);
    this->add("NIKON", "COOLPIX P7700", 7.44f, 5.58f, 4000, 3000);
    this->add("NIKON", "COOLPIX S800c", 6.17f, 4.55f, 4608, 3456);
    this->add("NIKON CORPORATION", "NIKON D3200", 23.2f, 15.4f, 6016, 4000);
    this->add("NIKON CORPORATION", "NIKON D800", 35.9f, 24.0f, 7360, 4912);
    this->add("NIKON CORPORATION", "NIKON D800E", 35.9f, 24.0f, 7360, 4912);
    this->add("NIKON", "COOLPIX P310", 6.17f, 4.55f, 4608, 3456);
    this->add("NIKON", "COOLPIX P510", 6.17f, 4.55f, 4608, 3456);
    this->add("NIKON", "COOLPIX S9300", 6.17f, 4.55f, 4608, 3456);
    this->add("NIKON CORPORATION", "NIKON D4", 36.0f, 23.9f, 4928, 3280);
    this->add("NIKON CORPORATION", "NIKON 1 V1", 13.2f, 8.8f, 3872, 2592);
    this->add("NIKON CORPORATION", "NIKON 1 J1", 13.2f, 8.8f, 3872, 2592);
    this->add("NIKON", "COOLPIX P7100", 7.44f, 5.58f, 3648, 2736);
    this->add("NIKON CORPORATION", "NIKON D5100", 23.6f, 15.7f, 4928, 3264);
    this->add("NIKON", "COOLPIX P300", 6.17f, 4.55f, 4000, 3000);
    this->add("NIKON", "COOLPIX S9100", 6.17f, 4.55f, 4000, 3000);
    this->add("NIKON CORPORATION", "NIKON D7000", 23.6f, 15.7f, 4928, 3264);
    this->add("NIKON CORPORATION", "NIKON D3100", 23.1f, 15.4f, 4608, 3072);
    this->add("NIKON", "COOLPIX P100", 6.17f, 4.55f, 3648, 2736);
    this->add("NIKON", "COOLPIX S8000", 6.17f, 4.55f, 4320, 3240);
    this->add("NIKON CORPORATION", "NIKON D3S", 36.0f, 23.9f, 4256, 2832);
    this->add("NIKON CORPORATION", "NIKON D3000", 23.6f, 15.8f, 3872, 2592);
    this->add("NIKON CORPORATION", "NIKON D300S", 23.6f, 15.8f, 4288, 2848);
    this->add("NIKON CORPORATION", "NIKON D5000", 23.6f, 15.8f, 4288, 2848);
    this->add("NIKON CORPORATION", "NIKON D3X", 35.9f, 24.0f, 6048, 4032);
    this->add("NIKON CORPORATION", "NIKON D90", 23.6f, 15.8f, 4288, 2848);
    this->add("NIKON CORPORATION", "NIKON D700", 36.0f, 24.0f, 4256, 2832);
    this->add("NIKON CORPORATION", "NIKON D60", 23.6f, 15.8f, 3872, 2592);
    this->add("NIKON CORPORATION", "NIKON D3", 36.0f, 23.9f, 4256, 2832);
    this->add("NIKON", "COOLPIX P5000", 7.144f, 5.358f, 3648, 2736);
    this->add("NIKON", "COOLPIX P7000", 7.44f, 5.58f, 3648, 2736);
    this->add("NIKON", "COOLPIX P50", 5.744f, 4.308f, 3264, 2448);
    this->add("NIKON", "COOLPIX P5100", 7.4f, 5.55f, 4000, 3000);
    this->add("NIKON CORPORATION", "NIKON D300", 23.6f, 15.8f, 4288, 2848);
    this->add("NIKON CORPORATION", "NIKON D40X", 23.7f, 15.6f, 3872, 2592);
    this->add("NIKON CORPORATION", "NIKON D40", 23.7f, 15.5f, 3008, 2000);
    this->add("NIKON", "COOLPIX S10", 5.744f, 4.308f, 2816, 2112);
    this->add("NIKON CORPORATION", "NIKON D80", 23.6f, 15.8f, 3872, 2592);
    this->add("NIKON", "COOLPIX P3", 7.144f, 5.358f, 3264, 2448);
    this->add("NIKON CORPORATION", "NIKON D200", 23.6f, 15.8f, 3872, 2592);
    this->add("NIKON CORPORATION", "NIKON D50", 23.7f, 15.5f, 3008, 2000);
    this->add("NIKON", "E7900", 7.144f, 5.358f, 3072, 2304);
    this->add("NIKON", "E4800", 5.744f, 4.308f, 2288, 1716);
    this->add("NIKON", "E8400", 8.8f, 6.6f, 3264, 2448);
    this->add("NIKON", "E8800", 8.8f, 6.6f, 3264, 2448);
    this->add("NIKON CORPORATION", "NIKON D2X", 23.7f, 15.7f, 4288, 2848);
    this->add("NIKON", "E5200", 7.144f, 5.358f, 2592, 1944);
    this->add("NIKON", "E8700", 8.8f, 6.6f, 3264, 2448);
    this->add("NIKON CORPORATION", "NIKON D70", 23.7f, 15.5f, 3008, 2000);
    this->add("NIKON CORPORATION", "NIKON D2H", 23.7f, 15.5f, 2464, 1632);
    this->add("NIKON", "E5400", 7.144f, 5.358f, 2592, 1944);
    this->add("NIKON", "E3100", 5.312f, 3.984f, 2048, 1536);
    this->add("NIKON", "E4500", 7.144f, 5.358f, 2272, 1704);
    this->add("NIKON", "E5700", 8.8f, 6.6f, 2560, 1920);
    this->add("NIKON", "E2500", 5.312f, 3.984f, 1600, 1200);
    this->add("NIKON CORPORATION", "NIKON D100", 23.7f, 15.5f, 3008, 2000);
    this->add("NIKON", "E5000", 8.8f, 6.6f, 2560, 1920);
    this->add("NIKON", "E885", 7.144f, 5.358f, 2048, 1536);
    this->add("NIKON", "E775", 5.312f, 3.984f, 1600, 1200);
    this->add("NIKON", "E995", 7.144f, 5.358f, 2048, 1536);
    this->add("NIKON CORPORATION", "NIKON D1H", 23.7f, 15.5f, 2000, 1312);
    this->add("NIKON", "E880", 7.144f, 5.358f, 2048, 1536);
    this->add("NIKON", "E990", 7.144f, 5.358f, 2048, 1536);
    this->add("NIKON", "E800", 6.4f, 4.8f, 1600, 1200);
    this->add("NIKON CORPORATION", "NIKON D1", 23.7f, 15.5f, 2000, 1312);

    /* Cameras from Olympus. */
    this->add("OLYMPUS IMAGING CORP.", "STYLUS1", 7.44f, 5.58f, 3968, 2976);
    this->add("OLYMPUS IMAGING CORP.", "E-M1", 17.3f, 13.0f, 4608, 3456);
    this->add("OLYMPUS IMAGING CORP.", "E-P5", 17.3f, 13.0f, 4608, 3456);
    this->add("OLYMPUS IMAGING CORP.", "TG-2", 6.17f, 4.55f, 3968, 2976);
    this->add("OLYMPUS IMAGING CORP.", "E-PL5", 17.3f, 13.0f, 4608, 3456);
    this->add("OLYMPUS IMAGING CORP.", "E-PM2", 17.3f, 13.0f, 4608, 3456);
    this->add("OLYMPUS IMAGING CORP.", "XZ-2", 7.44f, 5.58f, 3968, 2976);
    this->add("OLYMPUS IMAGING CORP.", "E-M5", 17.3f, 13.0f, 4608, 3456);
    this->add("OLYMPUS IMAGING CORP.", "E-P3", 17.3f, 13.0f, 4032, 3024);
    this->add("OLYMPUS IMAGING CORP.", "E-PL3", 17.3f, 13.0f, 4032, 3024);
    this->add("OLYMPUS IMAGING CORP.", "E-PM1", 17.3f, 13.0f, 4032, 3024);
    this->add("OLYMPUS IMAGING CORP.", "TG-810", 6.17f, 4.55f, 4288, 3216);
    this->add("OLYMPUS IMAGING CORP.", "VR320,D725", 6.17f, 4.55f, 4288, 3216);
    this->add("OLYMPUS IMAGING CORP.", "E-PL2", 17.3f, 13.0f, 4032, 3024);
    this->add("OLYMPUS IMAGING CORP.", "XZ-1", 8.07f, 5.56f, 3664, 2752);
    this->add("OLYMPUS IMAGING CORP.", "E-PL1", 17.3f, 13.0f, 4032, 3024);
    this->add("OLYMPUS IMAGING CORP.", "u9010,S9010", 6.08f, 4.56f, 4288, 3216);
    this->add("OLYMPUS IMAGING CORP.", "E-P2", 17.3f, 13.0f, 4032, 3024);
    this->add("OLYMPUS IMAGING CORP.", "E-620", 17.3f, 13.0f, 4032, 3024);
    this->add("OLYMPUS IMAGING CORP.", "u9000,S9000", 6.08f, 4.56f, 3968, 2976);
    this->add("OLYMPUS IMAGING CORP.", "E-30", 17.3f, 13.0f, 4032, 3024);
    this->add("OLYMPUS IMAGING CORP.", "E-420", 17.3f, 13.0f, 3648, 2736);
    this->add("OLYMPUS IMAGING CORP.", "E-P1", 17.3f, 13.0f, 4032, 3024);
    this->add("OLYMPUS IMAGING CORP.", "uT6000,ST6000", 6.17f, 4.55f, 3648, 2736);
    this->add("OLYMPUS IMAGING CORP.", "uT8000,ST8000", 6.08f, 4.56f, 3968, 2976);
    this->add("OLYMPUS IMAGING CORP.", "E-520", 17.3f, 13.0f, 3648, 2736);
    this->add("OLYMPUS IMAGING CORP.", "E-3", 17.3f, 13.0f, 3648, 2736);
    this->add("OLYMPUS IMAGING CORP.", "SP560UZ", 6.17f, 4.55f, 3264, 2448);
    this->add("OLYMPUS IMAGING CORP.", "E-410", 17.3f, 13.0f, 3648, 2736);
    this->add("OLYMPUS IMAGING CORP.", "E-510", 17.3f, 13.0f, 3648, 2736);
    this->add("OLYMPUS IMAGING CORP.", "SP550UZ", 5.744f, 4.308f, 3072, 2304);
    this->add("OLYMPUS IMAGING CORP.", "E-330", 17.3f, 13.0f, 3136, 2352);
    this->add("OLYMPUS IMAGING CORP.", "E-500", 17.3f, 13.0f, 3264, 2448);
    this->add("OLYMPUS IMAGING CORP.", "SP310", 7.144f, 5.358f, 3072, 2304);
    this->add("OLYMPUS IMAGING CORP.", "SP500UZ", 5.744f, 4.308f, 2816, 2112);
    this->add("OLYMPUS IMAGING CORP.", "uD800,S800", 7.144f, 5.358f, 3264, 2448);
    this->add("OLYMPUS IMAGING CORP.", "E-300", 17.3f, 13.0f, 3264, 2448);
    this->add("OLYMPUS IMAGING CORP.", "C70Z,C7000Z", 7.144f, 5.358f, 3072, 2304);
    this->add("OLYMPUS IMAGING CORP.", "u-miniD,Stylus V", 5.744f, 4.308f, 2272, 1704);
    this->add("OLYMPUS CORPORATION", "C8080WZ", 8.8f, 6.6f, 3264, 2448);
    this->add("OLYMPUS CORPORATION", "E-1", 17.3f, 13.0f, 2560, 1920);
    this->add("OLYMPUS OPTICAL CO.,LTD", "X-2,C-50Z", 7.144f, 5.358f, 2560, 1920);
    this->add("OLYMPUS OPTICAL CO.,LTD", "C5050Z", 7.144f, 5.358f, 2560, 1920);
    this->add("OLYMPUS OPTICAL CO.,LTD", "E-20,E-20N,E-20P", 8.8f, 6.6f, 2560, 1920);
    this->add("OLYMPUS OPTICAL CO.,LTD", "C40Z,D40Z", 7.144f, 5.358f, 2272, 1704);
    this->add("OLYMPUS OPTICAL CO.,LTD", "C700UZ", 5.312f, 3.984f, 1600, 1200);
    this->add("OLYMPUS OPTICAL CO.,LTD", "C3040Z", 7.144f, 5.358f, 2048, 1536);
    this->add("OLYMPUS OPTICAL CO.,LTD", "E-10", 8.8f, 6.6f, 2240, 1680);
    this->add("OLYMPUS OPTICAL CO.,LTD", "C2100UZ", 6.4f, 4.8f, 1600, 1200);
    this->add("OLYMPUS OPTICAL CO.,LTD", "C3030Z", 7.144f, 5.358f, 2048, 1536);

    /* Cameras from Panasonic. */
    this->add("Panasonic", "DMC-FH7", 6.08f, 4.56f, 4608, 3456);
    this->add("Panasonic", "DMC-FX01", 5.744f, 4.308f, 2816, 2112);
    this->add("Panasonic", "DMC-FX07", 5.744f, 4.308f, 3072, 2304);
    this->add("Panasonic", "DMC-FX3", 5.744f, 4.308f, 2816, 2112);
    this->add("Panasonic", "DMC-FX7", 5.744f, 4.308f, 2560, 1920);
    this->add("Panasonic", "DMC-FX8", 5.744f, 4.308f, 2560, 1920);
    this->add("Panasonic", "DMC-FX9", 5.744f, 4.308f, 2816, 2112);
    this->add("Panasonic", "DMC-FZ10", 5.744f, 4.308f, 2304, 1728);
    this->add("Panasonic", "DMC-FZ100", 6.08f, 4.56f, 4320, 3240);
    this->add("Panasonic", "DMC-FZ150", 6.17f, 4.55f, 4000, 3000);
    this->add("Panasonic", "DMC-FZ18", 5.744f, 4.308f, 3264, 2448);
    this->add("Panasonic", "DMC-FZ20", 5.744f, 4.308f, 2560, 1920);
    this->add("Panasonic", "DMC-FZ200", 6.17f, 4.55f, 4000, 3000);
    this->add("Panasonic", "DMC-FZ3", 4.544f, 3.408f, 2016, 1512);
    this->add("Panasonic", "DMC-FZ30", 7.144f, 5.358f, 3264, 2448);
    this->add("Panasonic", "DMC-FZ38", 6.08f, 4.56f, 4000, 3000);
    this->add("Panasonic", "DMC-FZ47", 6.08f, 4.56f, 4000, 3000);
    this->add("Panasonic", "DMC-FZ5", 5.744f, 4.308f, 2560, 1920);
    this->add("Panasonic", "DMC-FZ50", 7.144f, 5.358f, 3648, 2736);
    this->add("Panasonic", "DMC-FZ7", 5.744f, 4.308f, 2816, 2112);
    this->add("Panasonic", "DMC-FZ70", 6.17f, 4.55f, 4608, 3456);
    this->add("Panasonic", "DMC-FZ8", 5.744f, 4.308f, 3072, 2304);
    this->add("Panasonic", "DMC-G1", 17.3f, 13.0f, 4000, 3000);
    this->add("Panasonic", "DMC-G10", 17.3f, 13.0f, 4000, 3000);
    this->add("Panasonic", "DMC-G2", 17.3f, 13.0f, 4000, 3000);
    this->add("Panasonic", "DMC-G3", 17.3f, 13.0f, 4592, 3448);
    this->add("Panasonic", "DMC-G6", 17.3f, 13.0f, 4608, 3456);
    this->add("Panasonic", "DMC-GF1", 17.3f, 13.0f, 4000, 3000);
    this->add("Panasonic", "DMC-GF2", 17.3f, 13.0f, 4000, 3000);
    this->add("Panasonic", "DMC-GF3", 17.3f, 13.0f, 4000, 3000);
    this->add("Panasonic", "DMC-GF6", 17.3f, 13.0f, 4592, 3448);
    this->add("Panasonic", "DMC-GH1", 18.89f, 14.48f, 4000, 3000);
    this->add("Panasonic", "DMC-GH2", 17.3f, 13.0f, 4608, 3456);
    this->add("Panasonic", "DMC-GH3", 17.3f, 13.0f, 4608, 3456);
    this->add("Panasonic", "DMC-GM1", 17.3f, 13.0f, 4592, 3448);
    this->add("Panasonic", "DMC-GX1", 17.3f, 13.0f, 4592, 3448);
    this->add("Panasonic", "DMC-GX7", 17.3f, 13.0f, 4592, 3448);
    this->add("Panasonic", "DMC-L1", 17.3f, 13.0f, 3136, 2352);
    this->add("Panasonic", "DMC-L10", 17.3f, 13.0f, 3648, 2736);
    this->add("Panasonic", "DMC-LF1", 7.44f, 5.58f, 4000, 3000);
    this->add("Panasonic", "DMC-LX1", 8.498f, 4.78f, 3840, 2160);
    this->add("Panasonic", "DMC-LX2", 8.498f, 4.78f, 4224, 2376);
    this->add("Panasonic", "DMC-LX3", 8.07f, 5.56f, 3648, 2736);
    this->add("Panasonic", "DMC-LX5", 8.07f, 5.56f, 3648, 2736);
    this->add("Panasonic", "DMC-LX7", 7.44f, 5.58f, 3648, 2736);
    this->add("Panasonic", "DMC-LZ2", 5.744f, 4.308f, 2560, 1920);
    this->add("Panasonic", "DMC-FT1", 6.08f, 4.56f, 4000, 3000);
    this->add("Panasonic", "DMC-TS3", 6.08f, 4.56f, 4000, 3000);
    this->add("Panasonic", "DMC-TS5", 6.08f, 4.56f, 4608, 3456);
    this->add("Panasonic", "DMC-TZ1", 5.744f, 4.308f, 2560, 1920);
    this->add("Panasonic", "DMC-TZ3", 5.76f, 4.32f, 3072, 2304);
    this->add("Panasonic", "DMC-TZ5", 6.08f, 4.56f, 3456, 2592);
    this->add("Panasonic", "DMC-TZ6", 5.744f, 4.308f, 3648, 2736);
    this->add("Panasonic", "DMC-ZS10", 6.08f, 4.56f, 4320, 3240);
    this->add("Panasonic", "DMC-TZ25", 6.17f, 4.55f, 4000, 3000);
    this->add("Panasonic", "DMC-ZS20", 6.08f, 4.56f, 4320, 3240);
    this->add("Panasonic", "DMC-TZ7", 6.08f, 4.56f, 3648, 2736);
    this->add("Panasonic", "DMC-TZ8", 6.08f, 4.56f, 4000, 3000);
    this->add("Panasonic", "DMC-TZ10", 6.08f, 4.56f, 4000, 3000);
    this->add("Panasonic", "DMC-TZ18", 6.08f, 4.56f, 4320, 3240);

    /* Cameras from Pentax. */
    this->add("RICOH IMAGING COMPANY, LTD.", "PENTAX K-3", 23.5f, 15.6f, 6016, 4000);
    this->add("PENTAX", "PENTAX K-50", 23.7f, 15.7f, 4928, 3264);
    this->add("PENTAX", "PENTAX Q7", 7.44f, 5.58f, 4000, 3000);
    this->add("PENTAX", "PENTAX K-500", 23.7f, 15.7f, 4928, 3264);
    this->add("PENTAX RICOH IMAGING", "PENTAX WG-3 GPS", 6.17f, 4.55f, 4608, 3456);
    this->add("PENTAX RICOH IMAGING", "PENTAX WG-3 GPS", 6.17f, 4.55f, 4608, 3456);
    this->add("PENTAX RICOH IMAGING", "PENTAX MX-1", 7.44f, 5.58f, 4000, 3000);
    this->add("PENTAX", "PENTAX K-5 II", 23.7f, 15.7f, 4928, 3264);
    this->add("PENTAX", "PENTAX K-5 II s", 23.7f, 15.7f, 4928, 3264);
    this->add("PENTAX", "PENTAX K-30", 23.7f, 15.7f, 4928, 3264);
    this->add("PENTAX", "PENTAX K-01", 23.7f, 15.7f, 4928, 3264);
    this->add("PENTAX", "PENTAX Q", 6.17f, 4.55f, 4000, 3000);
    this->add("PENTAX", "PENTAX Optio WG-1 GPS", 6.17f, 4.55f, 4288, 3216);
    this->add("PENTAX", "PENTAX K-5", 23.7f, 15.7f, 4928, 3264);
    this->add("PENTAX", "PENTAX Optio RZ10", 6.08f, 4.56f, 4288, 3216);
    this->add("PENTAX", "PENTAX K-r", 23.6f, 15.8f, 4288, 2848);
    this->add("PENTAX", "PENTAX 645D", 44.0f, 33.0f, 7264, 5440);
    this->add("PENTAX", "PENTAX X90", 6.08f, 4.56f, 4000, 3000);
    this->add("PENTAX", "PENTAX K-x", 23.6f, 15.8f, 4288, 2848);
    this->add("PENTAX", "PENTAX K-7", 23.4f, 15.6f, 4672, 3104);
    this->add("PENTAX", "PENTAX Optio W80", 6.08f, 4.56f, 4000, 3000);
    this->add("PENTAX", "PENTAX K-m", 23.5f, 15.7f, 3872, 2592);
    this->add("PENTAX", "PENTAX Optio W60", 6.08f, 4.56f, 3648, 2736);
    this->add("PENTAX Corporation", "PENTAX K200D", 23.5f, 15.7f, 3872, 2592);
    this->add("PENTAX", "PENTAX K20D", 23.4f, 15.6f, 4672, 3104);
    this->add("PENTAX Corporation", "PENTAX K10D", 23.5f, 15.7f, 3872, 2592);
    this->add("PENTAX Corporation", "PENTAX Optio A20", 5.744f, 4.308f, 3648, 2736);
    this->add("PENTAX Corporation", "PENTAX Optio M20", 5.744f, 4.308f, 3072, 2304);
    this->add("PENTAX Corporation", "PENTAX K100D", 23.5f, 15.7f, 3008, 2008);
    this->add("PENTAX Corporation", "PENTAX K110D", 23.5f, 15.7f, 3008, 2008);
    this->add("PENTAX Corporation", "PENTAX *ist DL2", 23.5f, 15.7f, 3008, 2008);
    this->add("PENTAX Corporation", "PENTAX Optio A10", 7.144f, 5.358f, 3264, 2448);
    this->add("PENTAX Corporation", "PENTAX *ist DS", 23.5f, 15.7f, 3008, 2008);
    this->add("PENTAX Corporation", "PENTAX Optio S5i", 5.744f, 4.308f, 2560, 1920);
    this->add("PENTAX Corporation", "PENTAX Optio 750Z", 7.144f, 5.358f, 3056, 2296);
    this->add("PENTAX Corporation", "PENTAX *ist D", 23.5f, 15.7f, 3008, 2008);
    this->add("PENTAX Corporation", "PENTAX Optio 550", 7.144f, 5.358f, 2592, 1944);
    this->add("PENTAX Corporation", "PENTAX Optio S", 5.744f, 4.308f, 2048, 1536);
    this->add("Asahi Optical Co.,Ltd", "PENTAX Optio 430", 7.144f, 5.358f, 2240, 1680);
    this->add("Asahi Optical Co.,Ltd", "PENTAX Optio 330", 7.144f, 5.358f, 2048, 1536);

    /* Cameras from Ricoh. */
    this->add("PENTAX RICOH IMAGING", "GR", 23.7f, 15.7f, 4928, 3264);
    this->add("RICOH", "GR DIGITAL 4", 7.44f, 5.58f, 3648, 2736);
    this->add("RICOH", "GXR MOUNT A12", 23.6f, 15.7f, 4288, 2848);
    this->add("RICOH", "PX", 6.17f, 4.55f, 4608, 3072);
    this->add("RICOH", "CX5", 6.17f, 4.55f, 3648, 2736);
    this->add("RICOH", "GXR P10", 6.17f, 4.55f, 3648, 2736);
    this->add("RICOH", "CX3", 6.17f, 4.55f, 3648, 2736);
    this->add("RICOH", "GXR", 7.44f, 5.58f, 3648, 2736);
    this->add("RICOH", "GXR", 23.6f, 15.7f, 4288, 2848);
    this->add("RICOH", "GR DIGITAL 3", 7.44f, 5.58f, 3648, 2736);
    this->add("RICOH", "CX1", 6.17f, 4.55f, 3456, 2592);
    this->add("RICOH", "RICOH R8", 6.17f, 4.55f, 3648, 2736);
    this->add("RICOH", "Caplio GX100", 7.36f, 5.52f, 3648, 2736);
    this->add("RICOH", "GR Digital", 7.144f, 5.358f, 3264, 2448);

    /* Cameras from Samsung. */
    this->add("SAMSUNG", "NX2000", 23.5f, 15.7f, 5472, 3648);
    this->add("SAMSUNG", "NX300", 23.5f, 15.7f, 5472, 3648);
    this->add("SAMSUNG", "NX200", 23.5f, 15.7f, 5472, 3648);
    this->add("SAMSUNG", "NX100", 23.4f, 15.6f, 4592, 3056);
    this->add("SAMSUNG", "EX1", 7.44f, 5.58f, 3648, 2736);
    this->add("SAMSUNG", "SAMSUNG WB650 / VLUU WB650 / SAMSUNG WB660", 6.17f, 4.55f, 4000, 3000);
    this->add("SAMSUNG", "NX10", 23.4f, 15.6f, 4592, 3056);
    this->add("SAMSUNG", "WB5000/HZ25W", 6.08f, 4.56f, 4000, 3000);
    this->add("SAMSUNG", " SAMSUNG WB500 / VLUU WB500 / SAMSUNG HZ10W", 6.08f, 4.56f, 3648, 2432);
    this->add("SAMSUNG TECHWIN", "VLUU NV 7, NV 7", 5.744f, 4.308f, 3072, 2304);
    this->add("SAMSUNG TECHWIN", "VLUU NV10, NV10", 7.144f, 5.358f, 3648, 2736);
    this->add("SAMSUNG TECHWIN", "Pro 815", 8.8f, 6.6f, 3264, 2448);
    this->add("Samsung Techwin", "<Digimax V700 / Kenox V10>", 7.144f, 5.358f, 3072, 2304);
    this->add("SAMSUNG", "GT-I9100", 4.55f, 3.41f, 3264, 2448);

    /* Cameras from Sigma. */
    this->add("SIGMA", "SIGMA SD1", 24.0f, 16.0f, 4800, 3200);
    this->add("SIGMA", "SIGMA SD1", 24.0f, 16.0f, 4800, 3200);
    this->add("SIGMA", "SIGMA DP2", 20.7f, 13.8f, 2640, 1760);
    this->add("SIGMA", "SIGMA DP1", 20.7f, 13.8f, 2640, 1760);
    this->add("SIGMA", "SIGMA SD10", 20.7f, 13.8f, 2268, 1512);
    this->add("SIGMA", "SIGMA SD9", 20.7f, 13.8f, 2268, 1512);

    /* Cameras from Sony. */
    this->add("SONY", "ILCE-7R", 35.9f, 24.0f, 7360, 4912);
    this->add("SONY", "ILCE-7", 35.8f, 23.9f, 6000, 4000);
    this->add("SONY", "DSC-RX10", 13.2f, 8.8f, 5472, 3648);
    this->add("SONY", "ILCE-3000", 23.5f, 15.6f, 5456, 3632);
    this->add("SONY", "NEX-5T", 23.4f, 15.6f, 4912, 3264);
    this->add("SONY", "DSC-RX1R", 35.8f, 23.9f, 6000, 4000);
    this->add("SONY", "DSC-RX100M2", 13.2f, 8.8f, 5472, 3648);
    this->add("SONY", "NEX-3N", 23.5f, 15.6f, 4912, 3264);
    this->add("SONY", "NEX-6", 23.5f, 15.6f, 4912, 3264);
    this->add("SONY", "SLT-A99V", 35.8f, 23.8f, 6000, 4000);
    this->add("SONY", "DSC-RX1", 35.8f, 23.8f, 6000, 4000);
    this->add("SONY", "DSC-RX100", 13.2f, 8.8f, 5472, 3648);
    this->add("SONY", "NEX-F3", 23.4f, 15.6f, 4912, 3264);
    this->add("SONY", "SLT-A57", 23.5f, 15.6f, 4912, 3264);
    this->add("SONY", "DSC-HX200V", 6.17f, 4.55f, 4896, 3672);
    this->add("SONY", "DSC-HX20V", 6.17f, 4.55f, 4896, 3672);
    this->add("SONY", "NEX-5N", 23.4f, 15.6f, 4912, 3264);
    this->add("SONY", "NEX-7", 23.5f, 15.6f, 6000, 4000);
    this->add("SONY", "SLT-A77V", 23.5f, 15.6f, 6000, 4000);
    this->add("SONY", "SLT-A65V", 23.5f, 15.6f, 6000, 4000);
    this->add("SONY", "NEX-00", 23.4f, 15.6f, 4912, 3264);
    this->add("SONY", "SLT-A00", 23.5f, 15.6f, 4912, 3264);
    this->add("SONY", "DSC-HX9V", 6.17f, 4.55f, 4608, 3456);
    this->add("SONY", "DSC-HX100V", 6.17f, 4.55f, 4608, 3456);
    this->add("SONY", "DSC-TX10", 6.17f, 4.55f, 4608, 3456);
    this->add("SONY", "DSC-HX7V", 6.17f, 4.55f, 4608, 3456);
    this->add("SONY", "SLT-A55V", 23.5f, 15.6f, 4912, 3264);
    this->add("SONY", "SLT-A33", 23.5f, 15.6f, 4592, 3056);
    this->add("SONY", "DSLR-A580", 23.5f, 15.6f, 4912, 3264);
    this->add("SONY", "DSLR-A390", 23.5f, 15.7f, 4592, 3056);
    this->add("SONY", "NEX-5", 23.4f, 15.6f, 4592, 3056);
    this->add("SONY", "NEX-3", 23.4f, 15.6f, 4592, 3056);
    this->add("SONY", "DSC-H00", 6.17f, 4.55f, 4320, 3240);
    this->add("SONY", "DSC-HX5V", 6.104f, 4.578f, 3456, 2592);
    this->add("SONY", "DSLR-A850", 35.9f, 24.0f, 6048, 4032);
    this->add("SONY", "DSLR-A550", 23.4f, 15.6f, 4592, 3056);
    this->add("SONY", "DSLR-A380", 23.6f, 15.8f, 4592, 3056);
    this->add("SONY", "DSC-HX1", 6.104f, 4.578f, 3456, 2592);
    this->add("SONY", "DSLR-A900", 35.9f, 24.0f, 6048, 4032);
    this->add("SONY", "DSC-H20", 6.17f, 4.55f, 3648, 2736);
    this->add("SONY", "DSLR-A350", 23.6f, 15.8f, 4592, 3056);
    this->add("SONY", "DSC-T300", 6.17f, 4.55f, 3648, 2736);
    this->add("SONY", "DSC-H10", 5.744f, 4.308f, 3264, 2448);
    this->add("SONY", "DSLR-A200", 23.6f, 15.8f, 3872, 2592);
    this->add("SONY", "DSLR-A700", 23.5f, 15.6f, 4272, 2848);
    this->add("SONY", "DSC-H3", 5.744f, 4.308f, 3264, 2448);
    this->add("SONY", "DSC-W80", 5.744f, 4.308f, 3072, 2304);
    this->add("SONY", "DSC-H7", 5.744f, 4.308f, 3264, 2448);
    this->add("SONY", "DSC-H9", 5.744f, 4.308f, 3264, 2448);
    this->add("SONY", "DSLR-A100", 23.6f, 15.8f, 3872, 2592);
    this->add("SONY", "DSC-H2", 5.744f, 4.308f, 2816, 2112);
    this->add("SONY", "DSC-H5", 5.744f, 4.308f, 3072, 2304);
    this->add("SONY", "DSC-R1", 21.5f, 14.4f, 3888, 2592);
    this->add("SONY", "DSC-S90", 5.312f, 3.984f, 2304, 1728);
    this->add("SONY", "DSC-W7", 7.144f, 5.358f, 3072, 2304);
    this->add("SONY", "DSC-H1", 6.104f, 4.578f, 2592, 1944);
    this->add("SONY", "DSC-P200", 7.144f, 5.358f, 3072, 2304);
    this->add("SONY", "DSC-L1", 5.312f, 3.984f, 2304, 1728);
    this->add("SONY", "DSC-V3", 7.144f, 5.358f, 3072, 2304);
    this->add("SONY", "DSC-P150", 7.144f, 5.358f, 3072, 2304);
    this->add("SONY", "DSC-F88", 6.104f, 4.578f, 2592, 1944);
    this->add("SONY", "DSC-F828", 8.8f, 6.6f, 3264, 2448);
    this->add("SONY", "DSC-V1", 7.144f, 5.358f, 2592, 1944);
    this->add("SONY", "CYBERSHOT", 8.8f, 6.6f, 2560, 1920);
    this->add("SONY", "CYBERSHOT", 7.144f, 5.358f, 2272, 1704);
    this->add("SONY", "CYBERSHOT", 7.144f, 5.358f, 2048, 1536);
    this->add("SONY", "CYBERSHOT", 7.144f, 5.358f, 2048, 1536);
    this->add("SONY", "CYBERSHOT", 8.8f, 6.6f, 2560, 1920);
    this->add("SONY", "CYBERSHOT", 7.144f, 5.358f, 2272, 1704);
    this->add("SONY", "SONY", 5.312f, 3.984f, 1600, 1200);
    this->add("SONY", "SONY", 7.144f, 5.358f, 2048, 1536);
    this->add("SONY", "CYBERSHOT", 7.144f, 5.358f, 2048, 1536);
    this->add("SONY", "CYBERSHOT", 7.144f, 5.358f, 2048, 1536);
    this->add("SONY", "MAVICA", 5.312f, 3.984f, 1600, 1200);
    this->add("SONY", "CYBERSHOT", 7.144f, 5.358f, 1856, 1392);
    this->add("SONY", "CYBERSHOT", 7.144f, 5.358f, 2048, 1536);
    this->add("SONY", "CYBERSHOT", 6.4f, 4.8f, 1600, 1200);
    this->add("SONY", "CYBERSHOT", 6.4f, 4.8f, 1344, 1024);

    /* Cameras from LGE. */
    this->add("LGE", "Nexus 5", 4.536f, 3.416f, 3264, 2448);
    this->add("LGE", "Nexus 4", 3.68f, 2.76f, 3264, 2448);
}

SFM_NAMESPACE_END
