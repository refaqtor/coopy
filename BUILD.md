Building Coopy
==============

To build Coopy, use [CMake](http://www.cmake.org).  The essentials will compile just fine on all operating systems, using a gcc-family compiler.

Building the essentials on Linux:

    mkdir build && cd build && cmake .. && make

Use the CMake GUI to enable optional support for various kinds of spreadsheets or databases.  In the build directory, do:

    ccmake .

and look for the options starting with `USE_*`

Homebrew formula
----------------

On OSX there's a [homebrew formula available](https://github.com/paulfitz/homebrew/blob/coopy/Library/Formula/coopy.rb) that includes most commonly asked for dependencies.

Gnumeric support on Linux
-------------------------

Install gnumeric, `apt-get install gnumeric` or similar

* When compiling Coopy, turn on the `USE_GNUMERIC` flag with `ccmake .` in
  the build directory.

That's it!  Maybe.

Coopy links Gnumeric's core library for reading/writing spreadsheet
files.  ~~At the time of writing, that library is not exposed in a very
friendly way in most package managers.~~ The library is now exposed
in several package managers and in recent Debian.  If you are on
an older Ubuntu/Debian distribution, you may need to jump through the
following hoops.

* Install gnumeric, ideally through your package manager (e.g.
  `apt-get install gnumeric`)

* Install matching gnumeric source, ideally through your package manager (e.g.
  `apt-get source gnumeric`)

* If possible, install all build dependencies of gnumeric through your package
  manager (e.g `apt-get build-dep gnumeric`)

* Run `./configure` on your gnumeric source, do anything it needs
  you to do in order to get configured.
* In the `src` subdirectory of gnumeric source, do 
  `make gnumeric-paths.h`.  If having trouble, go ahead and do `make` in the 
  gnumeric source directory.
* That's it!  If we are lucky, there's no need to actually compile gnumeric, 
  we just need its header files.
* When compiling Coopy, turn on the `USE_GNUMERIC` flag with ccmake/cmake-gui
* Give the path to Gnumeric source in `GNUMERIC_ROOT` with ccmake/cmake-gui

Complete steps on Debian/Ubuntu:

    apt-get install gnumeric
    apt-get build-dep gnumeric
    apt-get source gnumeric
    cd gnumeric-*
    ./configure
    cd src
    make gnumeric-paths.h
    # Success! We now have all the header files needed.
    # The "../gnumeric-N.N.N" directory will be GNUMERIC_ROOT in CMake.

Gnumeric support on Windows
---------------------------

Not for the faint of heart!  We need to recompile gnumeric for windows.
This is done by cross-compiling on a Linux machine using mingw.
If you don't know what that means, you'll need to go learn about that first.

A lot of the steps described here involve fixes that have been submitted upstream
so your experience may vary.

First, get gnumeric from git:

    git clone git://git.gnome.org/gnumeric && cd gnumeric

Configure gnumeric - we won't actually be compiling under Linux, so
  dependencies can be skipped
  
    ./autogen.sh
    LIBSPREADSHEET_CFLAGS=UNUSED LIBSPREADSHEET_LIBS=UNUSED GTK_CFLAGS=UNUSED GTK_LIBS=UNUSED GNUMERIC_LIBS=UNUSED GNUMERIC_CFLAGS=UNUSED ./configure

Install jhbuild, go to `gnumeric/tools/win32` and do:

    make release

Now, things get messy.  Some fixes you may need to make (do `make release` again afterwards):

    # may need to add the "mv" line as below in "build" script:
    (
      download 'http://go.microsoft.com/fwlink/?LinkId=14188' 
      mv ${archive_dir}/index* ${archive_dir}/htmlhelp.exe
    )

    # zlib 1.2.5 gone from site, modify moduleset to replace:
      www.zlib.net
    with
      http://sourceforge.net/projects/libpng/files/zlib/1.2.5/

    # glib won't accept new libtool versions on the host - if it 
    # complains, hack autogen.sh in release/src/glib*:
      set 2.2*) to 2.*) in have_libtool test

    # moduleset:
      ftp://ftp.simplesystems.org/pub/libpng/png/src/
    becomes
      ftp://ftp.simplesystems.org/pub/libpng/png/src/history/libpng12/

    # moduleset:
      module="pixman-0.17.2.tar.gz"
      version="0.17.2"
    becomes:
      module="pixman-0.18.0.tar.gz"
      version="0.18.0"

    # moduleset - specify versions of goffice and gnumeric
      <autotools id="goffice">
         <branch tag="GOFFICE_0_8_16"/>
      <autotools id="gnumeric">
         <branch tag="GNUMERIC_1_10_16"/>

    # if gnumeric fails to install because of documentation problem:
      tools/win32/release/src/gnumeric/doc/C/chm$ touch gnumeric.chm

Once `tools/win32/release/deploy/bin/gnumeric.exe` exists, you know you are done
(we don't actually need that executable, but a library that comes with it).
Just tell Coopy about it:

* Turn on USE_GNUMERIC in coopy
* Set the GNUMERIC_ROOT in coopy to ../tools/win32/release/deploy
* Cross-compile Coopy (see scripts/make_packages.sh mingw for tips)
* You may need to force pkgconfig to look at gnumeric's libraries (`PKG_CONFIG_PATH=/home/paulfitz/cvs/gnumeric/gnumeric-1.10.16/tools/win32/release/deploy/lib/pkgconfig/ cmake .`)
* When testing binaries, you'll need to get the DLLs in release/deploy/bin into your path (or just copy them to Coopy's bin directory).

Bindings
---------

To make python/ruby bindings, first install the development 
libraries for the language of interest (a -dev or -devel package usually). 
Then set the environment variable Coopy_DIR to the directory in which Coopy is built, and Coopy_ROOT to the source directory, and do something like:

    cd $Coopy_ROOT
    mkdir -p build/python
    cd build/python
    cmake -DCREATE_PYTHON=TRUE ../../bindings
    make
    python example.py

