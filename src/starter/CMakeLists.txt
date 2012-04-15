project(starter)

include_directories(
    ${QT_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(clientstarter_SRCS clientstarter.cpp)
add_executable(clientstarter ${clientstarter_SRCS})
target_link_libraries(clientstarter ${QT_QTCORE_LIBRARY})
