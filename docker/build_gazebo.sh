#!/bin/bash
set -e

# DOWNLOAD GAZEBO
cd /gazebo

# BUILD GAZEBO
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE="Release" \
         -DCMAKE_INSTALL_PREFIX=/usr \
         -DENABLE_SSE4=True
make -j4

# INSTALL GAZEBO
make install
# make tests

# Clean the Docker image from Gazebo source code
#TODO /gazebo was added with ADD, so this step doesn't really remove it from the images size.
# it does remove the build artifacts though..
cd /
rm -rf /gazebo 
