#!/bin/bash
set -e
cd /sdformat-3.1.1

# BUILD SDFORMAT
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE="Release" \
         -DCMAKE_INSTALL_PREFIX=/usr \
         -DCMAKE_INSTALL_LIBDIR=lib
make -j8

# INSTALL SDFORMAT
make install
