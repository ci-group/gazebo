prefix=@CMAKE_INSTALL_PREFIX@
libdir=${prefix}/lib
includedir=${prefix}/include

Name: gazebo_transport
Description: Gazebo Transport Library
Version: @GAZEBO_VERSION@
Requires:
Libs: -L${libdir} -lgazebo_transport  -lgazebo_msgs
CFlags: -I${includedir} -I${includedir}/gazebo 
