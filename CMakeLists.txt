cmake_minimum_required(VERSION 3.8)

project(fbpanel)

file(GLOB FBPANEL_SOURCES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    panel/*.h panel/*.c
)

add_executable(fbpanel ${FBPANEL_SOURCES})
set_target_properties(fbpanel PROPERTIES LINKER_LANGUAGE C)

find_package(X11 REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MODULES REQUIRED gtk+-2.0 gmodule-2.0)

target_link_libraries(fbpanel
    PRIVATE
        -lm
        ${X11_LIBRARIES}
        ${MODULES_LIBRARIES}
)
target_include_directories(fbpanel
    PUBLIC
        panel .
    SYSTEM PRIVATE
        ${X11_INCLUDE_DIRS}
        ${MODULES_INCLUDE_DIRS}
)
