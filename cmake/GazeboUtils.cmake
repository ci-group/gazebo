
################################################################################
#APPEND_TO_CACHED_STRING(_string _cacheDesc [items...])
# Appends items to a cached list.
MACRO (APPEND_TO_CACHED_STRING _string _cacheDesc)
  FOREACH (newItem ${ARGN})
    SET (${_string} "${${_string}} ${newItem}" CACHE INTERNAL ${_cacheDesc} FORCE)
  ENDFOREACH (newItem ${ARGN})
  #STRING(STRIP ${${_string}} ${_string})
ENDMACRO (APPEND_TO_CACHED_STRING)
                 
################################################################################
# APPEND_TO_CACHED_LIST (_list _cacheDesc [items...]
# Appends items to a cached list.
MACRO (APPEND_TO_CACHED_LIST _list _cacheDesc)
  SET (tempList ${${_list}})
  FOREACH (newItem ${ARGN})
    LIST (APPEND tempList ${newItem})
  ENDFOREACH (newItem ${newItem})
  SET (${_list} ${tempList} CACHE INTERNAL ${_cacheDesc} FORCE)
ENDMACRO(APPEND_TO_CACHED_LIST)

###############################################################################
# Append sources to the server sources list
MACRO (APPEND_TO_SERVER_SOURCES)
  FOREACH (src ${ARGN})
    APPEND_TO_CACHED_LIST(gazeboserver_sources 
                          ${gazeboserver_sources_desc}                   
                          ${CMAKE_CURRENT_SOURCE_DIR}/${src})
  ENDFOREACH (src ${ARGN})
ENDMACRO (APPEND_TO_SERVER_SOURCES)

###############################################################################
# Append headers to the server headers list
MACRO (APPEND_TO_SERVER_HEADERS)
  FOREACH (src ${ARGN})
    APPEND_TO_CACHED_LIST(gazeboserver_headers
                          ${gazeboserver_headers_desc}                   
                          ${CMAKE_CURRENT_SOURCE_DIR}/${src})
    APPEND_TO_CACHED_LIST(gazeboserver_headers_nopath
                          "gazeboserver_headers_nopath"                   
                          ${src})
  ENDFOREACH (src ${ARGN})
ENDMACRO (APPEND_TO_SERVER_HEADERS)

###############################################################################
# Append sources to the sensor sources list
MACRO (APPEND_TO_SENSOR_SOURCES)
  FOREACH (src ${ARGN})
    APPEND_TO_CACHED_LIST(gazebosensor_sources 
                          ${gazebosensor_sources_desc}                   
                          ${CMAKE_CURRENT_SOURCE_DIR}/${src})
  ENDFOREACH (src ${ARGN})
ENDMACRO (APPEND_TO_SENSOR_SOURCES)

###############################################################################
# Append sources to the controller sources list
MACRO (APPEND_TO_CONTROLLER_SOURCES)
  FOREACH (src ${ARGN})
    APPEND_TO_CACHED_LIST(gazebocontroller_sources 
                          ${gazebocontroller_sources_desc}                   
                          ${CMAKE_CURRENT_SOURCE_DIR}/${src})
  ENDFOREACH (src ${ARGN})
ENDMACRO (APPEND_TO_CONTROLLER_SOURCES)


#################################################
# Macro to turn a list into a string (why doesn't CMake have this built-in?)
MACRO (LIST_TO_STRING _string _list)
    SET (${_string})
    FOREACH (_item ${_list})
      SET (${_string} "${${_string}} ${_item}")
    ENDFOREACH (_item)
    #STRING(STRIP ${${_string}} ${_string})
ENDMACRO (LIST_TO_STRING)

#################################################
# BUILD ERROR macro
macro (BUILD_ERROR)
  foreach (str ${ARGN})
    SET (msg "\t${str}")
    MESSAGE (STATUS ${msg})
    APPEND_TO_CACHED_LIST(build_errors "build errors" ${msg})
  endforeach ()
endmacro (BUILD_ERROR)

#################################################
# BUILD WARNING macro
macro (BUILD_WARNING)
  foreach (str ${ARGN})
    SET (msg "\t${str}" )
    MESSAGE (STATUS ${msg} )
    APPEND_TO_CACHED_LIST(build_warnings "build warning" ${msg})
  endforeach (str ${ARGN})
endmacro (BUILD_WARNING)

#################################################
macro (gz_add_library _name)
  add_library(${_name} SHARED ${ARGN})
  target_link_libraries (${_name} ${general_libraries})
endmacro ()

#################################################
macro (gz_add_executable _name)
  add_executable(${_name} ${ARGN})
  target_link_libraries (${_name} ${general_libraries})
endmacro ()


#################################################
macro (INSTALL_INCLUDES _subdir)
  install(FILES ${ARGN} DESTINATION ${INCLUDE_INSTALL_DIR}/${_subdir} COMPONENT headers)
endmacro()

#################################################
macro (INSTALL_LIBRARY _name)
  set_target_properties(${_name} PROPERTIES SOVERSION ${GAZEBO_MAJOR_VERSION} VERSION ${GAZEBO_VERSION})
  install (TARGETS ${_name} DESTINATION ${LIB_INSTALL_DIR} COMPONENT shlib)
endmacro ()

#################################################
macro (INSTALL_EXECUTABLE _name)
  set_target_properties(${_name} PROPERTIES VERSION ${GAZEBO_VERSION})
  install (TARGETS ${_name} DESTINATION ${BIN_INSTALL_DIR})
endmacro ()
