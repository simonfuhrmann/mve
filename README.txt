Introduction
======================================================================

This is a guideline how to compile and use FSSR, the Floating Scale
Surface Reconstruction software accompanying the following paper:

    Floating Scale Surface Reconstruction [PDF, 11MB]
    Simon Fuhrmann and Michael Goesele
    In: ACM ToG (Proceedings of ACM SIGGRAPH 2014).
    http://tinyurl.com/floating-scale-surface-recon

This software is based on MVE, the Multi-View Environment. It must be
downloaded and compiled prior to this software. You can get it here:

    http://www.gris.tu-darmstadt.de/projects/multiview-environment/

Building MVE and FSSR
======================================================================

The following commands should get you started:

    # Download and compile MVE
    git clone https://github.com/simonfuhrmann/mve.git
    make -j8 -C mve

    # Download and compile FSSR
    git clone https://github.com/simonfuhrmann/fssr.git
    make -j8 -C fssr

The binaries are then located and ready for execution here:

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
