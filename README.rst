AXE (An XCore Emulator)
.......................

:Stable release: Unreleased

:Status: Experimental

:Maintainer: https://github.com/rlsosborne

:Description: AXE is a fast open source simulator of the XMOS XS1 Architecture.

AXE is designed for fast simulation of XCore programs. AXE doesn't attempt to
be cycle accurate. Instruction execution timing is approximate. AXE is
experimental and likely to contain bugs.

Peripherals
===========
AXE supports emulation of the following peripherals:

* UART (RX only)
* SPI Flash (Read command only)
* Ethernet PHY supporting MII interface (see NETWORKING.rst)
* LCD Screen

Known Issues
============

* No simulation of debug.
* No support for configurable delays on ports / clocks.
* Minimal support for reading and writing of pswitch / sswitch registers.
* There are a few other miscellaneous instructions that are unimplemented.

Dependencies
============

* CMake_
* libelf_
* libxml2_
* libxslt_
* LLVM_
* Python_ (for running tests)

A `CMake superproject <https://github.com/rlsosborne/axe_superproject>`_ is
also available which can be used to download and build AXE and its
dependencies.

Optional Libraries
==================

AXE can be configured to link against the following libraries:

* SDL2_ (for LCD Screen peripheral)

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

.. _CMake: http://www.cmake.org
.. _libelf: http://www.mr511.de/software/english.html
.. _libxml2: http://www.xmlsoft.org
.. _libxslt: http://xmlsoft.org/XSLT
.. _LLVM: http://llvm.org
.. _Python: http://www.python.org
.. _SDL2: http://www.libsdl.org
