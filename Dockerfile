FROM ubuntu:18.04

# DEPENDENCIES
RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y \
                build-essential \
                cmake           \
                cppcheck        \
                xsltproc        \
                python          \
                mercurial       \
                curl            \
                qtbase5-dev     \
                libboost-system-dev          \
                libboost-filesystem-dev      \
                libboost-program-options-dev \
                libboost-regex-dev           \
                libboost-iostreams-dev       \
                libtinyxml2-dev              \
                libxml2-utils                \
                ruby-dev                     \
                ruby                         \
                libignition-math4-dev        \
                libignition-fuel-tools1-dev  \
                libignition-msgs-dev         \
                libignition-transport4-dev   \
                libqwt-dev

# SDFORMAT
COPY docker/build_sdformat.sh /tmp/build_sdformat.sh
RUN curl http://osrf-distributions.s3.amazonaws.com/sdformat/releases/sdformat-6.0.0.tar.bz2 | tar -xjC / \
  && /tmp/build_sdformat.sh \
  && rm -rf /sdformat-6.0.0

# GAZEBO
ADD . /gazebo
RUN /gazebo/docker/build_gazebo.sh
