/*
 * Copyright (C) 2016, Samir Aroudj
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef MVE_CAMERA_IO_HEADER
#define MVE_CAMERA_IO_HEADER

#include "mve/view.h"

MVE_NAMESPACE_BEGIN

/* ------------------- MVE native camera info format ------------------- */

/** Saves camera info of each view in the provided list to file_name. */
void
save_camera_infos (std::vector<View::Ptr> const& views, std::string const& file_name);


MVE_NAMESPACE_END

#endif /* MVE_CAMERA_IO_HEADER */

