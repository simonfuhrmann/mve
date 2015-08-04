Introduction
---------------------------------------------------------------------------

The Multi-View Environment is an effort to ease the work with multi-view
datasets and to support the development of algorithms based on multiple
views. It features Structure from Motion, Multi-View Stereo and Surface
Reconstruction. MVE is developed at the TU Darmstadt. Visit the following
websites for more details.

    http://www.gris.tu-darmstadt.de/projects/multiview-environment/
    http://www.gris.tu-darmstadt.de/
    http://www.tu-darmstadt.de/

This README.txt covers compilation and basic information about the
pipeline. For documentation, please refer to the Wiki pages on GitHub.

    https://github.com/simonfuhrmann/mve/wiki


Building MVE and UMVE
---------------------------------------------------------------------------

To download and build MVE, type:

    $ git clone https://github.com/simonfuhrmann/mve.git
    $ cd mve
    $ make -j8

To compile and run UMVE, the Qt user interface, type:

    $ cd apps/umve/
    $ qmake && make -j8
    $ ./umve

System requirements to compile and run MVE or UVME are:

    libjpeg (for MVE, http://www.ijg.org/)
    libpng (for MVE, http://www.libpng.org/pub/png/libpng.html)
    libtiff (for MVE, http://www.libtiff.org/)
    OpenGL (for libogl in MVE and UMVE)
    Qt 5 (for UMVE, http://qt.nokia.com)

Windows and OS X: Please refer to the Wiki pages for instructions.


The Reconstruction Pipeline
---------------------------------------------------------------------------

The MVE reconstruction pipeline is composed of the following components:

* Creating a dataset, by converting input photos into the MVE File Format.
* Structure from Motion, which reconstructs the camera parameters.
* Multi-View Stereo, which reconstructs dense depth maps for each photo.
* Surface Reconstruction, which reconstructs a surface mesh.

Please refer to the Wiki pages for a more detailed description.


Licensing
---------------------------------------------------------------------------

Most of MVE is licensed under the BSD 3-Clause license. Some parts are
licensed using the GPL 3 license. See the source file headers or the
LICENSE.txt file for more details.

