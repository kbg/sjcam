project(sjcserver)

include_directories(
    ${QT_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

set(sjcserver_SRCS
    sjcserver_main.cpp
    cmdlineopts.cpp
    sjcserver.cpp
    pvutils.cpp
    camera.cpp
    recorder.cpp
    imagestreamer.cpp
    imagewriter.cpp
)
qt4_automoc(${sjcserver_SRCS})
qt4_wrap_cpp(sjcserver_MOC_SRCS
    sjcserver.h
    recorder.h
    imagestreamer.h
    imagewriter.h
)

add_executable(sjcserver ${sjcserver_SRCS} ${sjcserver_MOC_SRCS})
target_link_libraries(sjcserver
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTNETWORK_LIBRARY}
    ${DCPCLIENT_LIBRARIES}
    ${PROSILICA_LIBRARIES}
    ${CFITSIO_LIBRARIES}
)

install(TARGETS sjcserver RUNTIME DESTINATION bin)
