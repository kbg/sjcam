# - Find DcpClient, based on FindJPEG.cmake by Kitware
# Find the DcpClient includes and library
# This module defines
#  DCPCLIENT_INCLUDE_DIR, where to find fitsio.h, etc.
#  DCPCLIENT_LIBRARIES, the libraries needed to use DcpClient.
#  DCPCLIENT_FOUND, If false, do not try to use DcpClient.
# also defined, but not for general use are
#  DCPCLIENT_LIBRARY, where to find the DcpClient library.

FIND_PATH(DCPCLIENT_INCLUDE_DIR dcpclient/dcpclient.h
    PATHS /usr/include /usr/local/include /opt/dcpclient/include
)

SET(DCPCLIENT_NAMES ${DCPCLIENT_NAMES} DcpClient)
FIND_LIBRARY(DCPCLIENT_LIBRARY NAMES ${DCPCLIENT_NAMES})

# handle the QUIETLY and REQUIRED arguments and set DCPCLIENT_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DCPCLIENT DEFAULT_MSG
    DCPCLIENT_LIBRARY DCPCLIENT_INCLUDE_DIR)

IF(DCPCLIENT_FOUND)
  SET(DCPCLIENT_LIBRARIES ${DCPCLIENT_LIBRARY})
ENDIF(DCPCLIENT_FOUND)

MARK_AS_ADVANCED(DCPCLIENT_LIBRARY DCPCLIENT_INCLUDE_DIR )
