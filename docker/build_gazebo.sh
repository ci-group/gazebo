#!/bin/bash
set -e

# DEPENDECIES
apt-get -y install libogre-1.9-dev                  \
                   libbullet-dev                    \
                   python-protobuf                  \
                   libprotoc-dev                    \
                   libprotobuf-dev                  \
                   libfreeimage-dev                 \
                   protobuf-compiler                \
                   libboost-thread-dev              \
                   libboost-signals-dev             \
                   libboost-system-dev              \
                   libboost-filesystem-dev          \
                   libboost-program-options-dev     \
                   libboost-regex-dev               \
                   libboost-iostreams-dev           \
                   freeglut3-dev                    \
                   libcurl4-openssl-dev             \
                   libtar-dev                       \
                   libtbb-dev                       \
                   libgts-dev                       \
                   uuid-dev                         \
                   libswscale-dev                   \
                   libavformat-dev                  \
                   libavcodec-dev                   \
                   libgraphviz-dev                  \
                   libhdf5-dev                      \
                   libopenal-dev                    \
                   pkg-config

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
cd /
rm -rf /gazebo

