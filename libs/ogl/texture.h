/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef OGL_TEXTURE_HEADER
#define OGL_TEXTURE_HEADER

#include <memory>

#include "mve/image.h"
#include "ogl/defines.h"
#include "ogl/opengl.h"

OGL_NAMESPACE_BEGIN

/**
 * OpenGL texture abstraction. 2D textures are supported only.
 */
class Texture
{
public:
    typedef std::shared_ptr<Texture> Ptr;
    typedef std::shared_ptr<Texture const> ConstPtr;

public:
    /** Creates a new texture object without image data. */
    Texture (void);
    /** Creates a new texture object from an MVE image. */
    Texture (mve::ByteImage::ConstPtr image);

    /** Creates a smart pointered texture object. */
    static Ptr create (void);

    /** Destroys the texture object, releasing OpenGL resources. */
    ~Texture (void);

    /** Makes this texture the active texture. */
    void bind (void);

    /** Uploads the given image to OpenGL. */
    void upload (mve::ByteImage::ConstPtr image);

private:
    GLuint tex_id;
};

/* ---------------------------------------------------------------- */

inline
Texture::Texture (void)
{
    glGenTextures(1, &this->tex_id);
}

inline
Texture::Texture (mve::ByteImage::ConstPtr image)
{
    glGenTextures(1, &this->tex_id);
    this->upload(image);
}

inline Texture::Ptr
Texture::create (void)
{
    return Ptr(new Texture);
}

inline
Texture::~Texture (void)
{
    glDeleteTextures(1, &this->tex_id);
}

inline void
Texture::bind (void)
{
    glBindTexture(GL_TEXTURE_2D, this->tex_id);
}

OGL_NAMESPACE_END

#endif /* OGL_TEXTURE_HEADER */
