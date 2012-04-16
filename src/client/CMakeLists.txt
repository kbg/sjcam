project(sjcclient)

include_directories(
    ${QT_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

set(sjcclient_SRCS
    sjcclient_main.cpp
    cmdlineopts.cpp
    sjcclient.cpp
    cameradock.cpp
    recordingdock.cpp
    histogramdock.cpp
    CamSys/ColorBar.cpp
    CamSys/ColorRange.cpp
    CamSys/ColorTable.cpp
    CamSys/Histogram.cpp
    CamSys/HistogramWidget.cpp
    CamSys/Image.cpp
    CamSys/ImageRenderer.cpp
    CamSys/ImageScrollArea.cpp
    CamSys/ImageWidget.cpp
)
qt4_automoc(${sjcclient_SRCS})
qt4_wrap_cpp(sjcclient_MOC_SRCS
    sjcclient.h
    cameradock.h
    recordingdock.h
    histogramdock.h
    CamSys/ColorBar.h
    CamSys/HistogramWidget.h
    CamSys/ImageScrollArea.h
    CamSys/ImageWidget.h
)
qt4_wrap_ui(sjcclient_UI_HDRS
    ui/sjcclient.ui
    ui/cameradock.ui
    ui/recordingdock.ui
    ui/histogramdock.ui
)

qt4_add_resources(sjcclient_RCC_SRCS ui/sjcclient.qrc)
set(sjcclient_WINRC ui/sjcclient.rc)

add_executable(sjcclient
    ${sjcclient_SRCS}
    ${sjcclient_MOC_SRCS}
    ${sjcclient_UI_HDRS}
    ${sjcclient_RCC_SRCS}
    ${sjcclient_WINRC}
)
target_link_libraries(sjcclient
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTNETWORK_LIBRARY}
    ${DCPCLIENT_LIBRARIES}
)

install(TARGETS sjcclient RUNTIME DESTINATION bin)
