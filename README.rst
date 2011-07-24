AXE (An XCore Emulator)
.......................

:Stable release: Unreleased

:Status: Experimental

:Maintainer: https://github.com/rlsosborne

:Description: AXE is a fast open source simulator of the XMOS XS1 Architecture.

AXE is designed for fast simulation of XCore programs. AXE doesn't attempt to
be cycle accurate. Instruction execution timing is approximate. AXE is
experimental and likely to contain bugs.

Known Issues
====================

* No simulation of the boot ROM.
* No simulation of debug.
* No support for configurable delays on ports / clocks.
* Minimal support for reading and writing of pswitch / sswitch registers.
* No support for partial inputs / output (INPW, OUTPW, SETPSC).
* There are a few other miscellaneous instructions that are unimplemented.
* The loopback option (--loopback) can only be used to connect ports on the
  first core. There is currently no way to connect ports on other cores.

Dependencies
============

* CMake
* libelf
* libxml2
* python (for running tests)

Building
========

Make a directory for the build and in that directory run::

  cmake -DCMAKE_BUILD_TYPE=Release <path-to-src>
  make

For a debug build use -DCMAKE_BUILD_TYPE=Debug. On Windows use nmake instead of
make.

Running tests
=============
The "check" target runs the testsuite. An install of the XMOS tools is required.
Run the SetupEnv script provided with the XMOS tools to add xcc to the path
before running the tests.
