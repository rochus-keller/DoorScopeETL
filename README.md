# DoorScopeEtl Application

DoorScopeEtl is an application which is run on the machine where the DOORS client runs. The application is responsible of receiving the data sent from within the DOORS client by the script exportToDoorScopeEtl2.dxl.

## Download and Installation
DoorScopeEtl is deployed as a compressed single-file executable. See https://github.com/DoorScope#downloads. The executable is built from the source code accessible here. Of course you can build the executable yourself if you want (see below for instructions). Since DoorScopeEtl is a single executable, it can just be downloaded and unpacked. No installation is necessary. You therefore need no special privileges to run DoorScopeEtl on your machine. 

## How to Build DoorScopeEtl

### Preconditions
DoorScopeEtl requires Qt4.x. The single-file executables are static builds based on Qt 4.4.3. 

You can download the Qt 4.4.3 source tree from here: http://download.qt.io/archive/qt/4.4/qt-all-opensource-src-4.4.3.tar.gz

The source tree also includes documentation and build instructions.

If you intend to do static builds on Windows without dependency on C++ runtime libs and manifest complications, follow the recommendations in this post: http://www.archivum.info/qt-interest@trolltech.com/2007-02/00039/Fed-up-with-Windows-runtime-DLLs-and-manifest-files-Here's-a-solution.html (link is dead)

Here is the summary on how to do implement Qt Win32 static builds:

1. in Qt/mkspecs/win32-msvc2005/qmake.conf replace MD with MT and MDd with MTd
2. in Qt/mkspecs/features clear the content of the two embed_manifest_*.prf files (but don't delete the files)
3. run configure -release -static -platform win32-msvc2005

### Build Steps
Follow these steps if you inted to build DoorScopeEtl yourself (don't forget to meet the preconditions before you start):

1. Create a directory; let's call it BUILD_DIR
2. Download the DoorScopeEtl source code from https://github.com/rochus-keller/DoorScopeEtl/archive/github.zip and unpack it to the BUILD_DIR.
3. Download the Stream source code from https://github.com/rochus-keller/Stream/archive/github.zip and unpack it to the BUILD_DIR; rename "Stream-github" to "Stream".
4. Goto the BUILD_DIR/DoorScope subdirectory and execute `QTDIR/bin/qmake DoorScopeEtl.pro` (see the Qt documentation concerning QTDIR).
5. Run make; after a couple of minutes you will find the executable in the tmp subdirectory.

Alternatively you can open DoorScopeEtl.pro using QtCreator and build it there.

## Support
If you need support or would like to post issues or feature requests please post an issue on GitHub.

