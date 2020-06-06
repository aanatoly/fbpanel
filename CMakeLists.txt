cmake_minimum_required(VERSION 3.5)

project(fbpanel VERSION 7.0)

include(GNUInstallDirs)

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

set(PLUGINS battery cpu deskno genmon image mem2 meter pager space tclock volume chart dclock deskno2 icons launchbar mem menu net separator taskbar tray user wincmd)

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

# we need this header in order to build target
configure_file (
    "${PROJECT_SOURCE_DIR}/config.h.in"
    "${PROJECT_SOURCE_DIR}/config.h"
)

# workout manpage
set(DATADIR "${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}/config")
configure_file (
    "${PROJECT_SOURCE_DIR}/data/man/fbpanel.1.in"
    "${PROJECT_BINARY_DIR}/fbpanel.1"
)

# workout default config
file(READ "${PROJECT_SOURCE_DIR}/data/config/default.in" defaultconfig)
string(REPLACE "%%datadir%%" "${CMAKE_INSTALL_FULL_DATAROOTDIR}" defaultconfig ${defaultconfig})
string(REPLACE "@libexecdir@" "${CMAKE_INSTALL_FULL_LIBEXECDIR}/${PROJECT_NAME}" defaultconfig ${defaultconfig})
file(WRITE "${PROJECT_BINARY_DIR}/default" ${defaultconfig})

# workout pager config
configure_file (
    "${PROJECT_SOURCE_DIR}/data/config/pager.in"
    "${PROJECT_BINARY_DIR}/pager"
)

# workout make_profile script
set(datadir "${CMAKE_INSTALL_FULL_DATAROOTDIR}/${PROJECT_NAME}")
configure_file (
    "${PROJECT_SOURCE_DIR}/exec/make_profile.in"
    "${PROJECT_BINARY_DIR}/make_profile"
    @ONLY
)

# create "make install" rules in one place
install(PROGRAMS "${PROJECT_BINARY_DIR}/${PROJECT_NAME}" DESTINATION bin)
foreach(PLUGIN ${PLUGINS})
    install(PROGRAMS "${PROJECT_BINARY_DIR}/lib${PLUGIN}.so" DESTINATION lib)
endforeach()
set(DOCS CHANGELOG CHANGELOG.md COPYING CREDITS INSTALL.md NOTES README.md)
foreach(DOC ${DOCS})
    install(FILES "${PROJECT_SOURCE_DIR}/${DOC}" DESTINATION "${CMAKE_INSTALL_DOCDIR}-${CMAKE_PROJECT_VERSION}")
endforeach()
install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.1" DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
install(DIRECTORY "${PROJECT_SOURCE_DIR}/data/images" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}" PATTERN "Makefile" EXCLUDE)
install(FILES "${PROJECT_BINARY_DIR}/default" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/config")
install(FILES "${PROJECT_BINARY_DIR}/pager" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/config")
install(PROGRAMS "${PROJECT_SOURCE_DIR}/exec/xlogout" DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}/${PROJECT_NAME}")
install(PROGRAMS "${PROJECT_BINARY_DIR}/make_profile" DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}/${PROJECT_NAME}")
install(FILES "${PROJECT_SOURCE_DIR}/po/fr_FR.UTF-8.mo" DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/locale/fr/LC_MESSAGES" RENAME "${PROJECT_NAME}.mo")
install(FILES "${PROJECT_SOURCE_DIR}/po/ru_RU.UTF-8.mo" DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/locale/ru/LC_MESSAGES" RENAME "${PROJECT_NAME}.mo")
