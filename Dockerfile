FROM ubuntu:18.04

# DEPENDENCIES
RUN apt-get update && apt-get upgrade -y && apt-get install -y \
        build-essential              \
        cmake                        \
        libavcodec-dev               \
        libavdevice-dev              \
        libavformat-dev              \
        libboost-filesystem-dev      \
        libboost-iostreams-dev       \
        libboost-program-options-dev \
        libboost-regex-dev           \
        libboost-system-dev          \
        libboost-thread-dev          \
        libbullet-dev                \
        libcurl4-openssl-dev         \
        libfreeimage-dev             \
        libgraphviz-dev              \
        libgts-dev                   \
        libhdf5-dev                  \
        libignition-fuel-tools1-dev  \
        libignition-math4-dev        \
        libignition-msgs-dev         \
        libignition-transport4-dev   \
        libogre-1.9-dev              \
        libopenal-dev                \
        libprotobuf-dev              \
        libprotoc-dev                \
        libqwt-dev                   \
        libsdformat6-dev             \
        libswscale-dev               \
        libtar-dev                   \
        libtbb-dev                   \
        libtinyxml2-dev              \
        pkg-config                   \
        protobuf-compiler            \
        qtbase5-dev                  \
        uuid-dev                     \
        xsltproc                  && \
    apt-get clean  && \
    rm -rf /var/lib/apt/lists/*

# GAZEBO
ADD . /gazebo
RUN /gazebo/docker/build_gazebo.sh
