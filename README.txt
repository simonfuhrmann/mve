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
    * Licensing
    * Hacking / Contributing to MVE


Building MVE / UMVE
======================================================================

Building is a two step process:

1. Building the libraries. In order to build the libraries, type "make" in
the base path or in the "libs/" directory. Note that the "ogl" library is
required for UMVE, but most applications do not require that library. (In
fact, as of now, only UMVE requires this library.) Building this library
will fail on systems without OpenGL, but this is fine as long as only
applications not requiring "ogl" are built afterwards.

2. Building the applications. Applications need to be build manually by
typing "make" in the corresponding directory of the application. For
some applications, "qmake" is used as build system. In these cases, such as
for UMVE, type "qmake" followed by "make" to build the application.

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

Building under MS Windows may work as well. However, documenting this
process is currently out of scope of this document.


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

Currently, there is not automated install procedure.

MVE as well as UMVE is not dependent on any external files except for the
UMVE shader. As such, you can easily install UMVE by copying the compiled
binary file to any directory in your $PATH, or linking to it. Make sure
that the "shader" directory is located just next to the binary.


Licensing
======================================================================

Please see the COPYING.txt file for licensing information.


Hacking MVE
======================================================================

Just a few notes on developing MVE itself.

HACKING. Developing library functionality is done by adding a new app to
the "apps/" directory (copy and edit a Makefile!), or in the "_test.cc"
file in one of the library directories ("make test" to build it). Due to
Makefile rules, files that start with an underscore ("_") are not build and
linked into the library.

TESTING. Unit tests are performed with googletest; it can be obtained from:
http://code.google.com/p/googletest/
In order to build and execute MVE unit tests, export GTEST_PATH to the path
of your local googletest copy, then: 1. cd $GTEST_PATH/make, 2. make,
3. rename "gtest_main.a" to "libgtest_main.a", 4. "make gtest" in MVE.

CONTRIBUTING. You are welcome to send in contributions to MVE that add new
functionality or fix bugs. Note that only contributions that meet the quality
standards of the libraries are likely to go into the official code base.
All contributions will be published under the same license as MVE (see
COPYING.txt for further details), and contributed code is up to future
changes in licensing or licensing to third parties on an individual basis.
