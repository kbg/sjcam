# - Find Prosilica, based on FindJPEG.cmake by Kitware
# Find the Prosilica includes and library
# This module defines
#  PROSILICA_INCLUDE_DIR, where to find PvApi.h, etc.
#  PROSILICA_LIBRARIES, the libraries needed to use PROSILICA.
#  PROSILICA_FOUND, If false, do not try to use PROSILICA.
# also defined, but not for general use are
#  PROSILICA_LIBRARY_PVAPI, where to find the PvApi library.
#  PROSILICA_LIBRARY_PTHREAD, where to find the pthread library.
#  PROSILICA_LIBRARY_RT, where to find the rt library.

FIND_PATH(PROSILICA_INCLUDE_DIR PvApi.h
    PATHS /usr/include /usr/local/include /opt/pvsdk/include
)

SET(PROSILICA_NAMES ${PROSILICA_NAMES} PvAPI pthread rt)
FIND_LIBRARY(PROSILICA_LIBRARY_PVAPI
    NAMES PvAPI
    PATHS /usr/lib /usr/local/lib
)
FIND_LIBRARY(PROSILICA_LIBRARY_RT
    NAMES rt
    PATHS /usr/lib /usr/local/lib
)
FIND_LIBRARY(PROSILICA_LIBRARY_PTHREAD
    NAMES pthread
    PATHS /usr/lib /usr/local/lib
)

# handle the QUIETLY and REQUIRED arguments and set PROSILICA_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PROSILICA DEFAULT_MSG
    PROSILICA_LIBRARY_PVAPI
    PROSILICA_LIBRARY_RT
    PROSILICA_LIBRARY_PTHREAD
    PROSILICA_INCLUDE_DIR
)

IF(PROSILICA_FOUND)
    SET(PROSILICA_LIBRARIES
        ${PROSILICA_LIBRARY_PVAPI}
        ${PROSILICA_LIBRARY_RT}
        ${PROSILICA_LIBRARY_PTHREAD}
    )
ENDIF(PROSILICA_FOUND)

MARK_AS_ADVANCED(
    PROSILICA_LIBRARY_PVAPI
    PROSILICA_LIBRARY_RT
    PROSILICA_LIBRARY_PTHREAD
    PROSILICA_INCLUDE_DIR
)
