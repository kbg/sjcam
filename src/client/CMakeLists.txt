project(sjcclient)

include_directories(
    ${QT_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(sjcclient_SRCS
    sjcclient_main.cpp
    sjcclient.cpp
)
qt4_automoc(${sjcclient_SRCS})
qt4_wrap_cpp(sjcclient_MOC_SRCS sjcclient.h)
qt4_wrap_ui(sjcclient_UI_HDRS sjcclient.ui)

add_executable(sjcclient
    ${sjcclient_SRCS}
    ${sjcclient_MOC_SRCS}
    ${sjcclient_UI_HDRS}
)
target_link_libraries(sjcclient
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTNETWORK_LIBRARY}
    ${DCPCLIENT_LIBRARIES}
)

install(TARGETS sjcclient RUNTIME DESTINATION bin)
