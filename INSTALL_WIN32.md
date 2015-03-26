# Installation on Windows

This documentation describes how to set up a workspace for trying to compile
Gazebo on Windows.  It does not actually work yet.

## Supported compilers

At this moment, compilation has been testewd on Windows 7 and is supported when
using [Visual Studio 2013](https://www.visualstudio.com/downloads/).
Patches for other versions are welcome.

## Installation

Totally experimental, using pre-compiled binaries in a local workspace.  To
make things easier, use a MinGW shell for your editing work (such as the [Git Bash Shell](https://msysgit.github.io/) with [Mercurial](http://tortoisehg.bitbucket.org/download/index.html)), and only use the
Windows `cmd` for configuring and building.  You might also need to
[disable the Windows firewall](http://windows.microsoft.com/en-us/windows/turn-windows-firewall-on-off#turn-windows-firewall-on-off=windows-7). Not sure about that.

1. Make a directory to work in, e.g.:

        mkdir gz-ws
        cd gz-ws

1. Download the following things into that directory:

    - [freeImage 3.x, slightly modified to build on VS2013](http://packages.osrfoundation.org/win32/deps/FreeImage-vc12-x64-release-debug.zip)
    - [boost 1.56.0](http://packages.osrfoundation.org/win32/deps/boost_1_56_0.zip)
    - [bzip2 1.0.6](http://packages.osrfoundation.org/win32/deps/bzip2-1.0.6-vc12-x64-release-debug.zip)
    - [dlfcn-win32](http://packages.osrfoundation.org/win32/deps/dlfcn-win32-vc12-x64-release-debug.zip)
    - [libcurl HEAD](http://packages.osrfoundation.org/win32/deps/libcurl-vc12-x64-release-debug-static-ipv6-sspi-winssl.zip)
    - [OGRE 1.9.0 rc1](http://packages.osrfoundation.org/win32/deps/ogre_src_v1-8-1-vc12-x64-release-debug.zip)
    - [protobuf 2.6.0](http://packages.osrfoundation.org/win32/deps/protobuf-2.6.0-win64-vc12.zip)
    - [TBB 4.3](http://packages.osrfoundation.org/win32/deps/tbb43_20141023oss_win.zip)
    - [zziplib 0.13.62](http://packages.osrfoundation.org/win32/deps/zziplib-0.13.62-vc12-x64-release-debug.zip)
    - [zlib](http://packages.osrfoundation.org/win32/deps/zlib-1.2.8-vc12-x64-release-debug.zip)

1. Unzip each of them in gz-ws.

1. Also download and execute the Qt 4.8 installer.  It should end up somewhere like C:\Qt.  As far as I can tell, this installation is not relocatable, so cannot be put into a .zip and dropped into a local workspace:

    - [Qt 4.8.6](http://packages.osrfoundation.org/win32/deps/qt-4.8.6-x64-msvc2013-rev1.7z)

1. Install cmake, make sure to select the "Add CMake to system path for all users" option in the install dialog box

    - [Cmake](http://www.cmake.org/download/)
    
1. Install Ruby 1.9 or greater. Make sure to have the installer add Ruby to your paths.

    - [Ruby](http://rubyinstaller.org/downloads/)
    
1. Clone sdformat and gazebo:

        hg clone https://bitbucket.org/osrf/sdformat
        hg clone https://bitbucket.org/osrf/gazebo

1. Load your compiler setup, e.g. (note that we are asking for the 64-bit toolchain here):

        "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86_amd64

1. Configure and build sdformat:

        cd sdformat
        mkdir build
        cd build
        ..\configure
        nmake
        nmake install

    You should now have an installation of sdformat in gz-ws/sdformat/build/install/Release.

1. Configure and build gazebo:

        cd gazebo
        mkdir build
        cd build
        ..\configure
        nmake
        nmake install

    Once this all works (which it currently does not, by a long shot): you should now have an installation of gazebo in gz-ws/sdformat/build/install/Release.

## Running

### gzserver

1. Adjust all paths to load dll
         cd gz-ws/gazebo
         addpath.bat Debug|Release

1. Download the ogre plugins.cfg file         
         cd gz-ws/gazebo/build/gazebo
         <download> http://packages.osrfoundation.org/win32/deps/plugins.cfg
         <edit> plugins.cfg
         gzserver.exe ..\..\worlds\empty.world

## Debugging

Just in case that you need to debug problems on Gazebo

### Building Ogre Examples

1. Download OIS
       http://sunet.dl.sourceforge.net/project/wgois/Source%20Release/1.3/ois-v1-3.zip
  
1. Compile OIS in Visual Studio
   Use the project in Win32/ folder

1. Place OIS headers and libs into
       ogre-.../Dependencies/include
       ogre-.../Dependencies/lib
       ogre-.../Dependencies/bin

1. Patch configure.bat inside ogre-1.8 to use
       -DOGRE_BUILD_SAMPLES:BOOL=TRUE .. 

1. Compile as usual
        ..\configure.bat
        nmake

1. Run the demo browser in
       # copy OIS_*.dll into the bin directory
       ogre-.../build/bin/SampleBrowser.exe


