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

target_include_directories(fbpanel
    PUBLIC
        panel .
)
target_link_libraries(fbpanel
    PRIVATE
        -lm
        ${X11_LIBRARIES}
        ${MODULES_LIBRARIES}
)
target_include_directories(fbpanel
    SYSTEM PRIVATE
        ${X11_INCLUDE_DIRS}
        ${MODULES_INCLUDE_DIRS}
)

set(PLUGINS "taskbar")

foreach(PLUGIN ${PLUGINS})

    file(GLOB PLUGIN_SOURCES
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        plugins/${PLUGIN}/*.c
    )

    add_library(${PLUGIN} SHARED ${PLUGIN_SOURCES})

    target_include_directories(${PLUGIN}
        PUBLIC
            panel .
    )
    target_link_libraries(${PLUGIN}
        PRIVATE
            ${MODULES_LIBRARIES}
    )
    target_include_directories(${PLUGIN}
        SYSTEM PRIVATE
            ${MODULES_INCLUDE_DIRS}
    )

endforeach()
