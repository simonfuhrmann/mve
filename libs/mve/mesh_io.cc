/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <stdexcept>

#include "util/string.h"
#include "mve/mesh.h"
#include "mve/mesh_io.h"
#include "mve/mesh_io_ply.h"
#include "mve/mesh_io_off.h"
#include "mve/mesh_io_npts.h"
#include "mve/mesh_io_pbrt.h"
#include "mve/mesh_io_smf.h"
#include "mve/mesh_io_obj.h"

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

TriangleMesh::Ptr
load_mesh (std::string const& filename)
{
    if (util::string::right(filename, 4) == ".off")
        return load_off_mesh(filename);
    else if (util::string::right(filename, 4) == ".ply")
        return load_ply_mesh(filename);
    else if (util::string::right(filename, 5) == ".npts")
        return load_npts_mesh(filename, false);
    else if (util::string::right(filename, 6) == ".bnpts")
        return load_npts_mesh(filename, true);
    else if (util::string::right(filename, 4) == ".smf")
        return load_smf_mesh(filename);
    else if (util::string::right(filename, 4) == ".obj")
        return load_obj_mesh(filename);
    else
        throw std::runtime_error("Extension not recognized");
}

/* ---------------------------------------------------------------- */

void
save_mesh (TriangleMesh::ConstPtr mesh, std::string const& filename)
{
    if (util::string::right(filename, 4) == ".off")
        save_off_mesh(mesh, filename);
    else if (util::string::right(filename, 4) == ".ply")
        save_ply_mesh(mesh, filename);
    else if (util::string::right(filename, 5) == ".pbrt")
        save_pbrt_mesh(mesh, filename);
    else if (util::string::right(filename, 5) == ".npts")
        save_npts_mesh(mesh, filename, false);
    else if (util::string::right(filename, 6) == ".bnpts")
        save_npts_mesh(mesh, filename, true);
    else if (util::string::right(filename, 4) == ".smf")
        save_smf_mesh(mesh, filename);
    else if (util::string::right(filename, 4) == ".obj")
        save_obj_mesh(mesh, filename);
    else
        throw std::runtime_error("Extension not recognized");
}

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END
