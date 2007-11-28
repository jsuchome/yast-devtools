#
# Find YaST and sets the following variables
# YAST_INCLUDE_DIR
# YAST_LIBRARY
# YAST_PLUGIN_DIR
# YAST_DATA_DIR
# YAST_PLUGIN_SCR_LIBRARY
# YAST_PLUGIN_WFM_LIBRARY
# YAST_YCP_LIBRARY
#

# set /usr as default prefix if not set
IF ( DEFINED CMAKE_INSTALL_PREFIX )
  MESSAGE(STATUS "prefix set to ${CMAKE_INSTALL_PREFIX}")
ELSE ( DEFINED CMAKE_INSTALL_PREFIX )
  SET(CMAKE_INSTALL_PREFIX /usr)
  MESSAGE(STATUS "No prefix, set to default /usr")
ENDIF ( DEFINED CMAKE_INSTALL_PREFIX )

# Library
IF ( DEFINED LIB )
  SET ( LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${LIB}" )
ELSE ( DEFINED  LIB )
  IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
    SET( LIB_SUFFIX "64" )
  ENDIF(CMAKE_SIZEOF_VOID_P MATCHES "8")
  SET ( LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}" )
ENDIF ( DEFINED  LIB )
MESSAGE(STATUS "Libraries will be installed in ${LIB_INSTALL_DIR}" )

if(YAST_INCLUDE_DIR AND YAST_LIBRARY AND YAST_YCP_LIBRARY)
  # Already in cache, be silent
  set(YAST_FIND_QUIETLY TRUE)  
endif(YAST_INCLUDE_DIR AND YAST_LIBRARY AND YAST_YCP_LIBRARY)

set(YAST_LIBRARY)
set(YAST_INCLUDE_DIR)
set(YAST_YCP_LIBRARY)

FIND_PATH(YAST_INCLUDE_DIR Y2.h
  ${CMAKE_INSTALL_PREFIX}/include/YaST2
  /usr/include/YaST2
  /usr/local/include/YaST2
)

SET(YAST_PLUGIN_DIR ${LIB_INSTALL_DIR}/YaST2/plugin)
SET(YAST_IMAGE_DIR ${CMAKE_INSTALL_PREFIX}/YaST2/plugin)
SET(YAST_DATA_DIR ${CMAKE_INSTALL_PREFIX}/share/YaST2/data)

FIND_LIBRARY(YAST_LIBRARY NAMES y2
  PATHS
  ${LIB_INSTALL_DIR}
  /usr/local/lib
)

FIND_LIBRARY(YAST_YCP_LIBRARY NAMES ycp
  PATHS
  ${LIB_INSTALL_DIR}
  /usr/local/lib
)

FIND_LIBRARY(YAST_PLUGIN_WFM_LIBRARY NAMES py2wfm
  PATHS
  ${YAST_PLUGIN_DIR}
  /usr/lib
  /usr/local/lib
)

FIND_LIBRARY(YAST_PLUGIN_SCR_LIBRARY NAMES py2scr
  PATHS
  ${YAST_PLUGIN_DIR}
  ${LIB_INSTALL_DIR}
  /usr/local/lib
)

SET(CMAKE_MODULE_PATH "${CMAKE_INSTALL_PREFIX}/share/YaST2/data/devtools/cmake/modules" ${CMAKE_MODULE_PATH})

if(YAST_INCLUDE_DIR AND YAST_LIBRARY AND YAST_YCP_LIBRARY)
   MESSAGE( STATUS "YaST2 found: includes in ${YAST_INCLUDE_DIR}, library in ${YAST_LIBRARY}")
   MESSAGE( STATUS "             plugins in ${YAST_PLUGIN_DIR}")
   MESSAGE( STATUS "             scr in ${YAST_PLUGIN_SCR_LIBRARY}")
   MESSAGE( STATUS "             wfm in ${YAST_PLUGIN_WFM_LIBRARY}")
   set(YAST_FOUND TRUE)
else(YAST_INCLUDE_DIR AND YAST_LIBRARY AND YAST_YCP_LIBRARY)
   MESSAGE( STATUS "YaST2 not found")
endif(YAST_INCLUDE_DIR AND YAST_LIBRARY AND YAST_YCP_LIBRARY)

MARK_AS_ADVANCED(YAST_INCLUDE_DIR YAST_LIBRARY YAST_YCP_LIBRARY YAST_PLUGIN_WFM_LIBRARY YAST_PLUGIN_SCR_LIBRARY YAST_PLUGIN_DIR YAST_IMAGE_DIR YAST_DATA_DIR)



