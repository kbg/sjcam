cmake_minimum_required(VERSION 2.8)
project(sjcam)

# Version informations
set(SJCAM_VERSION_MAJOR 0)
set(SJCAM_VERSION_MINOR 3)
set(SJCAM_VERSION_RELEASE 0)
set(SJCAM_VERSION_STRING "${SJCAM_VERSION_MAJOR}.${SJCAM_VERSION_MINOR}.${SJCAM_VERSION_RELEASE}")
set(SJCAM_COPYRIGHT_STRING "Copyright (c) 2012 Kolja Glogowski\\nKiepenheuer-Institut fuer Sonnenphysik")
configure_file(src/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Build options
option(BUILD_SERVER "Build Sjcam server." TRUE)
option(BUILD_CLIENT "Build Sjcam client." TRUE)
option(BUILD_STARTER "Build program starter." TRUE)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# Find libraries
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(Qt4 4.7.0 COMPONENTS QtCore QtGui QtNetwork REQUIRED)

find_package(DcpClient)
if(DCPCLIENT_FOUND)
    include_directories(${DCPCLIENT_INCLUDE_DIR})
    link_directories(${DCPCLIENT_LINK_DIRECTORIES})
else()
    message(FATAL_ERROR "DcpClient not found")
endif()

if(BUILD_SERVER)
    find_package(CFITSIO)
    if(CFITSIO_FOUND)
        include_directories(${CFITSIO_INCLUDE_DIR})
        link_directories(${CFITSIO_LINK_DIRECTORIES})
    else()
        message(FATAL_ERROR "CFITSIO not found")
    endif()

    find_package(Prosilica)
    if(PROSILICA_FOUND)
        # Defines needed for PvApi.h
        set(PROSILICA_OS LINUX CACHE STRING "The target operating sytem, see PvAPI.h")
        set(PROSILICA_CPU x64 CACHE STRING "The target cpu type, see PvAPI.h")
        mark_as_advanced(PROSILICA_OS PROSILICA_CPU)
        add_definitions(-D_${PROSILICA_OS} -D_${PROSILICA_CPU})
        include_directories(${PROSILICA_INCLUDE_DIR})
        link_directories(${PROSILICA_LINK_DIRECTORIES})
    else()
        message(FATAL_ERROR "Prosilica API not found")
    endif()
endif()

# Build
add_subdirectory(src)
