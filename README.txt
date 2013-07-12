Introduction
======================================================================

This is the MVE users guide for building, installation and operation.
MVE is developed at the TU Darmstadt, specifically in the GRIS department,
for more details visit the following websites:

    http://www.gris.tu-darmstadt.de/projects/multiview-environment/
    http://www.gris.tu-darmstadt.de/
    http://www.tu-darmstadt.de/

The libraries and applications has been developed by, or received
contributions from, the following persons:

    Simon Fuhrmann <simon.fuhrmann at gris.tu-darmstadt.de>
    Ronny Klowsky <ronny.klowsky at gris.tu-darmstadt.de>
    Jens Ackermann <jens.ackermann at gris.tu-darmstadt.de>
    Sebastian Koch <sebastian.koch at gris.tu-darmstadt.de>
    Fabian Langguth <fabian.langguth at gris.tu-darmstadt.de>

This README file covers some basic information regarding MVE.

    * Building MVE / UMVE
    * Running MVE / UMVE
    * Installing MVE / UMVE
    * Hacking / Contributing to MVE


Building MVE / UMVE
======================================================================

Building is a two step process:

1. Building the libraries. In order to build the libraries, type "make" in
the base path or in the "libs/" directory. Note that the "ogl" library is
required for UMVE, but most applications do not require that library. (In
fact, as of now, only UMVE requires this library.) Building this library
will fail on systems without OpenGL, and this is fine as long as "ogl" is
not required.

2. Building the applications. Applications need to be build manually by
typing "make" in the corresponding directory of the application. For
some applications (like UMVE), "qmake" is used as build system. In these
cases type "qmake" followed by "make" to build the application.

Requirements to compile and run MVE / UVME are:

    - libjpeg (for MVE, http://www.ijg.org/)
    - libpng (for MVE, http://www.libpng.org/pub/png/libpng.html)
    - libtiff (for MVE, http://www.libtiff.org/)
    - OpenGL (only for libogl in MVE and UMVE)
    - QT 4 (only for UMVE, http://qt.nokia.com)

To conclude, building and executing under Linux and OSX is as easy as:

    # <download and untar>
    # cd mve
    # make
    # cd apps/umve/
    # qmake && make -j8
    # ./umve

Consult the users guide in the MVE wiki for further information:
https://github.com/simonfuhrmann/mve/wiki/The-MVE-Users-Guide


Running MVE / UMVE
======================================================================

MVE is bundled with a few applications. Most of these applications operate
on MVE scenes, which need to be created beforehand. MVE scenes can be
created using the "makescene" application. Other applications can executed
without a scene, such as the "meshconvert" application. However, there is
lots of functionality in the libraries which is not directly accessible via
an explicit application.

UMVE is the QT-based GUI front-end application for easy management of
scenes, and requires "qmake" for the building process. Note that also UMVE
does not support creation of scenes. Use "makescene" for that.

Apart from UMVE, most time-consuming operations are also available as
console applications and can be executed on systems without graphical user
interface. One example is "dmrecon", which is the console version of the
MVS reconstruction. The "mveshell" is a command line alternative for UMVE,
but does not yet cover all of its functionality.


Installing MVE / UMVE
======================================================================

Currently, there is no install procedure.

MVE does not depend on any external files. UMVE only requires access to
the shaders, and expects these files in the "shader" directory located
next to the binary. Thus, it is easy to install UMVE by copying the binary
and the shader directory anywhere you want.


Hacking MVE
======================================================================

A few notes on developing MVE itself.

HACKING. Developing library functionality is done by adding code and the
corresponding unit tests. Sometimes it is difficult to test a feature and
a "_test.cc" file in one of the library can be used ("make test" to build
it). Due to Makefile rules, files that start with an underscore ("_") are
ignored when building the libraries.

TESTING. Unit tests are performed with GoogleTest, which can be obtained
from http://code.google.com/p/googletest/. More information is available
here: https://github.com/simonfuhrmann/mve/wiki/The-MVE-Developers-Guide

CONTRIBUTING. You are welcome to send in contributions to MVE that add new
functionality or fix bugs. By submitting code to MVE, you consent that all
contributions will be published under the same license as MVE (see
COPYING.txt for details), and all contributed code is up to future changes in
licensing or licensing to third parties on an individual basis. If you want
to license the code for commercial purposes, please get in contact with us.
