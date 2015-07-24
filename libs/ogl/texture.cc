/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include "ogl/texture.h"

OGL_NAMESPACE_BEGIN

void
Texture::upload (mve::ByteImage::ConstPtr image)
{

    GLint level = 0;
    GLint int_format = GL_RGBA;
    GLsizei w(image->width()), h(image->height()), c(image->channels());
    GLint border = 0;

    /* Set image properties. */
    void* data = (void*)const_cast<uint8_t*>(image->get_data_pointer());
    GLint type = GL_UNSIGNED_BYTE;
    GLint format = GL_RGBA;
    switch (c)
    {
        case 1: format = GL_RED; break;
        case 2: format = GL_RG; break;
        case 3: format = GL_RGB; break;
        case 4: format = GL_RGBA; break;
        default:
            throw std::invalid_argument("Invalid amount of image channels");
    }

#if 0
    std::cout << "Level: " << level << ", internal format: " << int_format
        << ", size: " << w << "x" << h << ", format: " << format
        << std::endl;
#endif

    this->bind();
    glTexImage2D(GL_TEXTURE_2D, level, int_format, w, h,
        border, format, type, data);
    glGenerateMipmap(GL_TEXTURE_2D);
}

OGL_NAMESPACE_END
