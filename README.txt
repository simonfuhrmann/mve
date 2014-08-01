Introduction
======================================================================

This is a guideline how to compile and use FSSR, the Floating Scale
Surface Reconstruction software accompanying the following paper:

    Floating Scale Surface Reconstruction
    Simon Fuhrmann and Michael Goesele
    In: ACM ToG (Proceedings of ACM SIGGRAPH 2014).
    http://tinyurl.com/floating-scale-surface-recon

This software is based on MVE, the Multi-View Environment. See the link below
for details on MVE, and the next section for downloading and building MVE.

    http://www.gris.tu-darmstadt.de/projects/multiview-environment/


Downloading and Building MVE and FSSR
======================================================================

MVE requires libjpeg, libpng and libtiff as dependencies, which can be obtained
using the package system of your distribution.

The following commands should get you started:

    # Download and compile MVE
    git clone https://github.com/simonfuhrmann/mve.git
    make -j8 -C mve

    # Download and compile FSSR
    git clone https://github.com/simonfuhrmann/fssr.git
    make -j8 -C fssr

The binaries will be located and ready for execution here:

    fssr/apps/fssr_octree
    fssr/apps/fssr_surface


FSSR Input Data
======================================================================

FSSR requires as input a point set which contains several required and
some optional attributes per sample. Currently, only PLY is supported as
input format. All required and optional attributes are listed below:

    Name           PLY Attribute(s)   Type                 Required?
    ------------------------------------------------------------------
    3D positions   x, y, z            float 3-vector       required
    normals        nx, ny, nz         float 3-vector       required
    scale values   value              single float value   required
    confidence     confidence         single float value   optional
    color          red, green, blue   uchar 3-vector       optional

A typical PLY header (without color) then looks like this:

    ply
    format binary_little_endian 1.0
    element vertex 36228
    property float x
    property float y
    property float z
    property float nx
    property float ny
    property float nz
    property float confidence
    property float value
    end_header

If you are using MVE and want to create a surface from a set of depth maps,
you can use the 'scene2pset' tool, which is included in the MVE distribution.

    Usage: scene2pset [ OPTS ] SCENE_DIR MESH_OUT

Make sure the parameters --depth and --image specify the correct depth map
and corresponding image. Also enable normals, scale and confidence with the
parameters --with-normals, --with-scale and --with-conf.


Running FSSR
======================================================================

First, the implicit function defined by the input points is sampled and
stored in an octree. This is done with the 'fssr_octree' tool.

    Usage: fssr_octree [ OPTS ] IN_PLY [ IN_PLY ... ] OUT_OCTREE

It takes one or more PLY files as input (connectivity information is ignored)
and samples the implicit function over an octree hierarchy. The result is
stored in a binary format. (Check the code for details.)

Second, the sampling of the implicit function must be converted to a
surface mesh. This is done with a second 'fssr_surface' tool.

    Usage: fssr_surface [ OPTS ] IN_OCTREE OUT_PLY_MESH

The tool takes as input the octree and produces a mesh. Several options
can control the resulting mesh. See the tools output for details.


Trouble? Contact!
======================================================================

Contact information can be found on the FSSR and MVE websites.
