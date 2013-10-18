#!/bin/bash

# This script will run an API/ABI compliance check on your working copy of
# Gazebo.  Along the way, it will install a system version of Gazebo (by
# default, the latest available).  You'll be prompted as needed for sudo access
# and to confirm the installation steps.
#
# We assume that your system is already configured to use the OSRF apt 
# repository (build.osrfoundation.org).  If you haven't done that step yet, read
# about it here:
#   http://gazebosim.org/wiki/2.0/install#Ubuntu_Debians

USAGE="api_checker.bash <gazebo_source_dir>"

# Stop on error
set -e

# Parse args
if [[ $# -lt 1 ]]; then
  echo $USAGE
  exit 1
else
  # Convert to absolute path, in case the developer gave something like '..'
  GAZEBO_SOURCE_DIR=$(readlink -f $1)
  GAZEBO_BRANCH=$(cd $GAZEBO_SOURCE_DIR && hg branch)
fi

# Install Gazebo and some tools we'll need
sudo apt-get update
sudo apt-get install gazebo exuberant-ctags git 
# Also install things that are needed to build Gazebo.  We want to ensure 
# that we end up with the same set of system packages, especially the optional
# ones, that are used in building the Gazebo .deb.  For now, I'm copying in the
# Build-Depends from precise/debian/control.  That's not ideal, but will
# probably work.
sudo apt-get install cmake \
     debhelper \
     doxygen \
     doxygen-latex \
     libfreeimage-dev \
     libprotoc-dev \
     libprotobuf-dev \
     protobuf-compiler \
     freeglut3-dev \
     libcurl4-openssl-dev \
     libtinyxml-dev \
     libtar-dev \
     libtbb-dev \
     libogre-dev \
     libxml2-dev \
     pkg-config \
     libqt4-dev \
     libltdl-dev \
     libboost-thread-dev \
     libboost-signals-dev \
     libboost-system-dev \
     libboost-filesystem-dev \
     libboost-program-options-dev \
     libboost-regex-dev \
     libboost-iostreams-dev \
     robot-player-dev \
     libcegui-mk2-dev \
     libavformat-dev \
     libavcodec-dev \
     libswscale-dev \
     sdformat

# Install abi-compliance-checker.git
TMPDIR=$(mktemp -d)
mkdir $TMPDIR/source $TMPDIR/build $TMPDIR/install $TMPDIR/config
cd $TMPDIR/source
git clone git://github.com/lvc/abi-compliance-checker.git  
cd abi-compliance-checker
perl Makefile.pl -install --prefix=$TMPDIR/install

# Build and install Gazebo.  I'm doing this as a separate build to avoid
# unexpected interactions with the developer's own build space.  An
# optimization would be to allow reuse of an existing build space.
cd $TMPDIR/build
cmake -DENABLE_TESTS_COMPILATION:BOOL=False -DCMAKE_INSTALL_PREFIX=$TMPDIR/install $GAZEBO_SOURCE_DIR
# Assume that we can use all cores
MAKE_JOBS=$(grep -c ^processor /proc/cpuinfo)
make -j${MAKE_JOBS}
make install

# Figure out which libraries we're going to compare and what version of Gazebo
# we're working with.
GAZEBO_LIBS=$(dpkg -L gazebo | grep lib.*.so)
GAZEBO_LIBS_LOCAL=$(dpkg -L gazebo | grep lib.*.so | sed -e "s:^/usr:$TMPDIR/install:g")
BIN_VERSION=$(dpkg -l gazebo | tail -n 1 | awk '{ print  $3 }')
MAJOR_MINOR=$(echo $BIN_VERSION | cut -d . -f 1,2)

# Generate the spec for our reference version of Gazebo.
cat > $TMPDIR/config/pkg.xml << CURRENT_DELIM
 <version>
     .deb pkg version: $BIN_VERSION
 </version>

 <headers>
   /usr/include/gazebo-$MAJOR_MINOR/gazebo
 </headers>

 <skip_headers>
   /usr/include/gazebo-$MAJOR_MINOR/gazebo/GIMPACT
   /usr/include/gazebo-$MAJOR_MINOR/gazebo/opcode
   /usr/include/gazebo-$MAJOR_MINOR/gazebo/test
 </skip_headers>

 <libs>
  $GAZEBO_LIBS
 </libs>
CURRENT_DELIM

# Generate the spec for our development version of Gazebo.
cat > $TMPDIR/config/devel.xml << DEVEL_DELIM
 <version>
     branch: $GAZEBO_BRANCH
 </version>
 
  <headers>
   $TMPDIR/install/include/gazebo-$MAJOR_MINOR/gazebo
 </headers>
 
 <skip_headers>
   $TMPDIR/install/include/gazebo-$MAJOR_MINOR/gazebo/GIMPACT
   $TMPDIR/install/include/gazebo-$MAJOR_MINOR/gazebo/opcode
   $TMPDIR/install/include/gazebo-$MAJOR_MINOR/gazebo/test
 </skip_headers>
 
 <libs>
  $GAZEBO_LIBS_LOCAL
 </libs>
DEVEL_DELIM

# Run the check
cd $TMPDIR/config
$TMPDIR/install/bin/abi-compliance-checker -lib gazebo -old pkg.xml -new devel.xml || true

echo "Your compatibility reports are in: "
echo "  $TMPDIR/config/compat_reports."
echo "When you're done, you might want to remove the working directory:"
echo "  $TMPDIR"

