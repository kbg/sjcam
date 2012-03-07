# - Find CFITSIO, based on FindJPEG.cmake by Kitware
# Find the CFITSIO includes and library
# This module defines
#  CFITSIO_INCLUDE_DIR, where to find fitsio.h, etc.
#  CFITSIO_LIBRARIES, the libraries needed to use CFITSIO.
#  CFITSIO_FOUND, If false, do not try to use CFITSIO.
# also defined, but not for general use are
#  CFITSIO_LIBRARY, where to find the CFITSIO library.

FIND_PATH(CFITSIO_INCLUDE_DIR fitsio.h
    PATHS /usr/include /usr/local/include
)

SET(CFITSIO_NAMES ${CFITSIO_NAMES} cfitsio)
FIND_LIBRARY(CFITSIO_LIBRARY NAMES ${CFITSIO_NAMES})

# handle the QUIETLY and REQUIRED arguments and set CFITSIO_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CFITSIO DEFAULT_MSG
    CFITSIO_LIBRARY CFITSIO_INCLUDE_DIR)

IF(CFITSIO_FOUND)
  SET(CFITSIO_LIBRARIES ${CFITSIO_LIBRARY})
ENDIF(CFITSIO_FOUND)

MARK_AS_ADVANCED(CFITSIO_LIBRARY CFITSIO_INCLUDE_DIR )
