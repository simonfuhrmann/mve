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
    make -j12 -C mve

    # Download and compile FSSR
    git clone https://github.com/simonfuhrmann/fssr.git
    make -j12 -C fssr

The binaries will be located here:

    fssr/apps/fssrecon
    fssr/apps/meshclean


FSSR Input Data
======================================================================

FSSR requires as input a point set which contains several required and
some optional attributes per sample. Currently, only PLY is supported as
input and output format. The attributes are listed below:

    Name           PLY Attribute(s)   Type                 Required?
    ------------------------------------------------------------------
    3D positions   x, y, z            float 3-vector       required
    normals        nx, ny, nz         float 3-vector       required
    scale values   value              single float value   required
    confidence     confidence         single float value   optional
    color          red, green, blue   uchar 3-vector       optional

A typical PLY file header looks like this:

    ply
    format binary_little_endian 1.0
    element vertex 36228
    property float x
    property float y
    property float z
    property float nx
    property float ny
    property float nz
    property uchar red
    property uchar green
    property uchar blue
    property float confidence
    property float value
    end_header

If you are using MVE and want to create a surface from a set of depth maps,
you can use the 'scene2pset' tool, which is included in the MVE distribution.

    Usage: scene2pset [ OPTS ] SCENE_DIR MESH_OUT

Make sure to specify the parameter --fssr=N which selects the correct depth
map and image at scale N, and includes normals, scale and confidences.


Running FSSR
======================================================================

First, the implicit function defined by the input samples is evaluated
and the isosurface is extracted. This is done with the 'fssrecon' tool.

    Usage: fssrecon [ OPTS ] IN_PLY [ IN_PLY ... ] OUT_PLY

It takes one or more PLY files as input (connectivity information is ignored)
and samples the implicit function over an octree hierarchy. The isosurface
is extracted and stored in a PLY mesh.

Second, the resulting mesh may contain low-confidence vertices, many small
isolated components, and degenerated triangles from the isosurfacing. The
resulting mesh should be cleaned with the 'meshclean' tool.

    Usage: meshclean [ OPTS ] IN_PLY OUT_PLY

The tool takes as input a mesh, and produces a cleaned output mesh. Several
options can control the resulting mesh. See the tools output for details.


Trouble? Contact!
======================================================================

Contact information can be found on the FSSR and MVE websites.
