/*
 * Copyright (C) 2015, Ronny Klowsky, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <cassert>
#include <string>

#include "mve/image_tools.h"
#include "mve/image_io.h"
#include "dmrecon/mvs_tools.h"

MVS_NAMESPACE_BEGIN

namespace
{
    /**
     * Lookup table that implements the conversion from RGB in [0..255]
     * to sRGB in [0..1] using the following specification:
     *
     * f(x) = (x / 255.0 / 12.92)                 if x <= 0.04045 * 255
     *        ((x / 255.0 + 0.055) / 1.055)^2.4   otherwise
     *
     */
    float const srgb2lin[256] = {
        0.0f, 0.000303526991f, 0.000607053982f, 0.000910580973f,
        0.00121410796f, 0.00151763496f, 0.00182116195f, 0.00212468882f,
        0.00242821593f, 0.0027317428f, 0.00303526991f, 0.00334653584f,
        0.00367650739f, 0.00402471703f, 0.00439144205f, 0.00477695325f,
        0.00518151652f, 0.00560539169f, 0.00604883302f, 0.00651209056f,
        0.00699541019f, 0.00749903219f, 0.00802319311f, 0.00856812578f,
        0.00913405884f, 0.00972121768f, 0.010329823f, 0.0109600937f,
        0.0116122449f, 0.012286488f, 0.0129830325f, 0.0137020834f,
        0.0144438436f, 0.0152085144f, 0.0159962941f, 0.0168073755f,
        0.0176419541f, 0.01850022f, 0.0193823613f, 0.0202885624f,
        0.0212190095f, 0.0221738853f, 0.0231533665f, 0.0241576321f,
        0.0251868591f, 0.0262412224f, 0.0273208916f, 0.02842604f,
        0.0295568351f, 0.0307134446f, 0.0318960324f, 0.0331047662f,
        0.0343398079f, 0.0356013142f, 0.0368894488f, 0.0382043719f,
        0.0395462364f, 0.0409151986f, 0.0423114114f, 0.043735031f,
        0.045186203f, 0.0466650873f, 0.0481718257f, 0.0497065671f,
        0.0512694567f, 0.0528606474f, 0.054480277f, 0.0561284907f,
        0.0578054301f, 0.0595112368f, 0.0612460524f, 0.0630100146f,
        0.064803265f, 0.0666259378f, 0.0684781671f, 0.0703600943f,
        0.0722718537f, 0.0742135718f, 0.0761853829f, 0.078187421f,
        0.0802198201f, 0.0822827071f, 0.0843762085f, 0.0865004584f,
        0.0886555836f, 0.0908417106f, 0.0930589661f, 0.0953074694f,
        0.097587347f, 0.0998987257f, 0.102241732f, 0.104616486f,
        0.107023105f, 0.10946171f, 0.111932427f, 0.114435375f,
        0.116970666f, 0.119538426f, 0.122138776f, 0.124771819f,
        0.127437681f, 0.130136475f, 0.13286832f, 0.135633335f,
        0.138431609f, 0.141263291f, 0.144128472f, 0.147027269f,
        0.149959788f, 0.152926147f, 0.155926466f, 0.158960834f,
        0.162029371f, 0.165132195f, 0.168269396f, 0.171441108f,
        0.174647406f, 0.177888423f, 0.18116425f, 0.18447499f,
        0.187820777f, 0.191201687f, 0.194617838f, 0.198069319f,
        0.20155625f, 0.205078736f, 0.208636865f, 0.212230757f,
        0.215860501f, 0.219526201f, 0.223227963f, 0.226965874f,
        0.230740055f, 0.23455058f, 0.238397568f, 0.242281124f,
        0.246201321f, 0.25015828f, 0.254152089f, 0.258182853f,
        0.262250662f, 0.266355604f, 0.270497799f, 0.274677306f,
        0.278894275f, 0.283148736f, 0.287440836f, 0.291770637f,
        0.296138257f, 0.300543785f, 0.304987311f, 0.309468925f,
        0.313988715f, 0.318546772f, 0.323143214f, 0.327778101f,
        0.332451522f, 0.337163627f, 0.341914415f, 0.346704066f,
        0.351532608f, 0.356400132f, 0.361306787f, 0.366252601f,
        0.371237695f, 0.376262128f, 0.38132602f, 0.386429429f,
        0.391572475f, 0.396755219f, 0.401977777f, 0.407240212f,
        0.412542611f, 0.417885065f, 0.423267663f, 0.428690493f,
        0.434153646f, 0.439657182f, 0.445201188f, 0.450785786f,
        0.456411034f, 0.462076992f, 0.467783809f, 0.473531485f,
        0.479320168f, 0.48514995f, 0.491020858f, 0.496932983f,
        0.502886474f, 0.50888133f, 0.514917672f, 0.520995557f,
        0.527115107f, 0.533276379f, 0.539479494f, 0.545724452f,
        0.55201143f, 0.558340371f, 0.564711511f, 0.571124852f,
        0.577580452f, 0.584078431f, 0.590618849f, 0.597201765f,
        0.603827357f, 0.610495567f, 0.617206573f, 0.623960376f,
        0.630757153f, 0.637596846f, 0.644479692f, 0.651405632f,
        0.658374846f, 0.665387273f, 0.672443151f, 0.679542482f,
        0.686685324f, 0.693871737f, 0.701101899f, 0.708375752f,
        0.715693474f, 0.723055124f, 0.730460763f, 0.73791039f,
        0.745404184f, 0.752942204f, 0.760524511f, 0.768151164f,
        0.775822222f, 0.783537805f, 0.791297913f, 0.799102724f,
        0.806952238f, 0.814846575f, 0.822785735f, 0.830769897f,
        0.838799f, 0.846873224f, 0.854992628f, 0.863157213f,
        0.871367097f, 0.8796224f, 0.887923121f, 0.896269381f,
        0.904661179f, 0.913098633f, 0.921581864f, 0.930110872f,
        0.938685715f, 0.947306514f, 0.955973327f, 0.964686275f,
        0.973445296f, 0.982250571f, 0.991102099f, 1.0f };

}

void
colAndExactDeriv(mve::ByteImage const& img, PixelCoords const& imgPos,
    PixelCoords const& gradDir, Samples& color, Samples& deriv)
{
    int const width = img.width();
    int const height = img.height();
    for (std::size_t i = 0; i < imgPos.size(); ++i)
    {
        int const left = std::floor(imgPos[i][0]);
        int const top = std::floor(imgPos[i][1]);
        float const x = imgPos[i][0] - left;
        float const y = imgPos[i][1] - top;

        if (left < 0 || left > width-1 || top < 0 || top > height-1)
            throw std::runtime_error("Image position out of bounds");

        /* data position pointer */
        int p0 = (top * width + left) * 3;
        int p1 = ((top+1) * width + left) * 3;

        /* bilinear interpolation to determine color value */
        float x0, x1, x2, x3, x4, x5;
        x0 = (1.f-x) * srgb2lin[img.at(p0  )] + x * srgb2lin[img.at(p0+3)];
        x1 = (1.f-x) * srgb2lin[img.at(p0+1)] + x * srgb2lin[img.at(p0+4)];
        x2 = (1.f-x) * srgb2lin[img.at(p0+2)] + x * srgb2lin[img.at(p0+5)];
        x3 = (1.f-x) * srgb2lin[img.at(p1  )] + x * srgb2lin[img.at(p1+3)];
        x4 = (1.f-x) * srgb2lin[img.at(p1+1)] + x * srgb2lin[img.at(p1+4)];
        x5 = (1.f-x) * srgb2lin[img.at(p1+2)] + x * srgb2lin[img.at(p1+5)];

        color[i][0] = (1.f - y) * x0 + y * x3;
        color[i][1] = (1.f - y) * x1 + y * x4;
        color[i][2] = (1.f - y) * x2 + y * x5;

        /* derivative in direction gradDir */
        float u = gradDir[i][0];
        float v = gradDir[i][1];
        for (int c = 0; c < 3; ++c)
        {
            deriv[i][c] =
                u * (srgb2lin[img.at(p0+3)] - srgb2lin[img.at(p0)])
                + v * (srgb2lin[img.at(p1)] - srgb2lin[img.at(p0)])
                + (v * x + u * y) *
                    (srgb2lin[img.at(p0)] - srgb2lin[img.at(p0+3)]
                     - srgb2lin[img.at(p1)] + srgb2lin[img.at(p1+3)]);
            ++p0;
            ++p1;
        }
    }
}

/* ------------------------------------------------------------------ */

void
getXYZColorAtPix(mve::ByteImage const& img,
    std::vector<math::Vec2i> const& imgPos, Samples* color)
{
    int width = img.width();
    Samples::iterator itCol = color->begin();

    for (std::size_t i = 0; i < imgPos.size(); ++i)
    {
        int const idx = imgPos[i][1] * width + imgPos[i][0];
        (*itCol)[0] = srgb2lin[img.at(idx,0)];
        (*itCol)[1] = srgb2lin[img.at(idx,1)];
        (*itCol)[2] = srgb2lin[img.at(idx,2)];
        ++itCol;
    }
}

/* ------------------------------------------------------------------ */

void
getXYZColorAtPos(mve::ByteImage const& img, PixelCoords const& imgPos,
    Samples* color)
{
    int width = img.width();
    int height = img.height();
    PixelCoords::const_iterator citPos;
    Samples::iterator itCol = color->begin();

    for (citPos = imgPos.begin(); citPos != imgPos.end(); ++citPos, ++itCol)
    {
        int const i = floor((*citPos)[0]);
        int const j = floor((*citPos)[1]);
        assert(i < width-1 && j < height-1);

        float const u = (*citPos)[0] - i;
        float const v = (*citPos)[1] - j;
        int const p0 = (j * width + i) * 3;
        int const p1 = ((j+1) * width + i) * 3;
        float x0, x1, x2, x3, x4, x5;
        x0 = (1.f-u) * srgb2lin[img.at(p0  )] + u * srgb2lin[img.at(p0+3)];
        x1 = (1.f-u) * srgb2lin[img.at(p0+1)] + u * srgb2lin[img.at(p0+4)];
        x2 = (1.f-u) * srgb2lin[img.at(p0+2)] + u * srgb2lin[img.at(p0+5)];
        x3 = (1.f-u) * srgb2lin[img.at(p1  )] + u * srgb2lin[img.at(p1+3)];
        x4 = (1.f-u) * srgb2lin[img.at(p1+1)] + u * srgb2lin[img.at(p1+4)];
        x5 = (1.f-u) * srgb2lin[img.at(p1+2)] + u * srgb2lin[img.at(p1+5)];

        (*itCol)[0] = (1.f - v) * x0 + v * x3;
        (*itCol)[1] = (1.f - v) * x1 + v * x4;
        (*itCol)[2] = (1.f - v) * x2 + v * x5;
    }
}

MVS_NAMESPACE_END
