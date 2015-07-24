/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef DMFUSION_STANFORD_HEADER
#define DMFUSION_STANFORD_HEADER

#include <string>
#include <vector>

#include "mve/mesh.h"
#include "mve/image.h"

#include "range_image.h"

/**
 * Reads the Stanford alignment file and performs the transformation.
 * Stanford alignment file format:
 *
 * camera T1 T2 T3 Q1 Q2 Q3 Q4
 * bmesh FILE_NAME T1 T2 T3 Q1 Q2 Q3 Q4
 * bmesh ...
 */
class StanfordAlignment
{
public:
    typedef std::vector<RangeImage> RangeImages;

public:
    StanfordAlignment (void);

    void read_alignment (std::string const& conffile);
    RangeImages const& get_range_images (void) const;
    mve::TriangleMesh::Ptr get_mesh (RangeImage const& ri);

private:
    RangeImages images;
};

/* ---------------------------------------------------------------- */

inline
StanfordAlignment::StanfordAlignment (void)
{
}

inline StanfordAlignment::RangeImages const&
StanfordAlignment::get_range_images (void) const
{
    return this->images;
}

#endif /* DMFUSION_STANFORD_HEADER */
