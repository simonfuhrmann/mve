# Introduction

The Multi-View Environment is an effort to ease the work with multi-view
datasets and to support the development of algorithms based on multiple
views. It features Structure from Motion, Multi-View Stereo and Surface
Reconstruction. MVE is developed at the TU Darmstadt. Visit the following
website for more details.

 * https://www.gcc.tu-darmstadt.de/home/proj/mve/

This README covers compilation and basic information about the
pipeline. For documentation, please refer to the Wiki pages on GitHub.

 * https://github.com/simonfuhrmann/mve/wiki


# Building MVE and UMVE

To download and build MVE, type:

    $ git clone https://github.com/simonfuhrmann/mve.git
    $ cd mve
    $ make -j8

To compile and run UMVE, the Qt user interface, type:

    $ cd apps/umve/
    $ qmake && make -j8
    $ ./umve

System requirements to compile and run MVE or UVME are:

 * libjpeg (for MVE, http://www.ijg.org/)
 * libpng (for MVE, http://www.libpng.org/pub/png/libpng.html)
 * libtiff (for MVE, http://www.libtiff.org/)
 * OpenGL (for libogl in MVE and UMVE)
 * Qt 5 (for UMVE, http://www.qt.io)

Windows and OS X: Please refer to the Wiki pages for instructions.


# The Reconstruction Pipeline

The MVE reconstruction pipeline is composed of the following components:

 * Creating a dataset, by converting input photos into the MVE File Format.
 * Structure from Motion, which reconstructs the camera parameters.
 * Multi-View Stereo, which reconstructs dense depth maps for each photo.
 * Surface Reconstruction, which reconstructs a surface mesh.

The reconstruction tools can be found under `mve/apps/`. Please refer to the
[MVE Users Guide](https://github.com/simonfuhrmann/mve/wiki/MVE-Users-Guide)
for a more detailed description how to use these tools. Note that UMVE is
merely an interface for scene inspection and does not support reconstuction.


# Licensing

See the LICENSE.txt file and the source file headers for more details.
