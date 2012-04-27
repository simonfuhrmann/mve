#include <iostream>

#include "math/matrix.h"

#include "mve/camera.h"
#include "mve/image.h"
#include "mve/imagefile.h"

int main (void)
{
    float const w = 3072.0f;
    float const h = 2048.0f;
    float aspect = w / h;
    float paspect = 2759.48f / 2764.16f;
    //float iaspect  = paspect * aspect;
    float flen = 2759.48f / w;
    float pp1 = 1520.69f / w;
    float pp2 = 1006.81f / h;


    //mve::ByteImage::Ptr img(mve::image::load_file("/tmp/0000.tiff"));
    mve::CameraInfo ci;
    ci.flen = flen;
    ci.ppoint[0] = 0.5f;//pp1;
    ci.ppoint[1] = 0.5f;//pp2;
    ci.paspect = paspect;

    math::Matrix3f mat;
    ci.fill_projection(*mat, w, h);
    math::Matrix3f imat;
    ci.fill_inverse_projection(*imat, w, h);

    std::cout << "Mult: "  << std::endl << (mat * imat) << std::endl;

    std::cout << "Mat:\n" << mat << std::endl;

    math::Vec3f p(0,0,-1);
    math::Vec3f ret = mat * p;
    std::cout << "Ret: " << ret << std::endl;
    ret /= ret[2];
    std::cout << "Ret: " << ret << std::endl;

    return 0;
}
