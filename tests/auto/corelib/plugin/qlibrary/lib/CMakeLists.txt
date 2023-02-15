# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## mylib Generic Library:
#####################################################################

qt_internal_add_cmake_library(mylib
    SHARED
    INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/tst_qlibrary"
    SOURCES
        mylib.c
    LIBRARIES
        Qt::Core
)

set_target_properties(mylib PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    C_VISIBILITY_PRESET "default"
    CXX_VISIBILITY_PRESET "default"
)

if(WIN32)
    # CMake sets for Windows-GNU platforms the suffix "lib"
    set_property(TARGET mylib PROPERTY PREFIX "")
endif()

if(UNIX)
    if(APPLE)
        add_custom_command(TARGET mylib POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:mylib>
                "${CMAKE_CURRENT_BINARY_DIR}/../libmylib.1.0.0.dylib"
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                "libmylib.1.0.0.dylib"
                "${CMAKE_CURRENT_BINARY_DIR}/../libmylib.1.dylib"
            VERBATIM)
    elseif(NOT ANDROID)
        add_custom_command(TARGET mylib POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:mylib>
                "${CMAKE_CURRENT_BINARY_DIR}/../libmylib.so.1.0.0"
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                "libmylib.so.1.0.0"
                "${CMAKE_CURRENT_BINARY_DIR}/../libmylib.so.1"
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                "libmylib.so.1.0.0"
                "${CMAKE_CURRENT_BINARY_DIR}/../libmylib.so1"
            VERBATIM)
    else()
        # Android does not use symlinks. Also, according to our conventions,
        # libraries on Android MUST be named in the following pattern:
        # lib*.so
        add_custom_command(TARGET mylib POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:mylib>
            "${CMAKE_CURRENT_BINARY_DIR}/../libmylib.so"
            VERBATIM)
    endif()
else() #Win32
    add_custom_command(TARGET mylib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:mylib>
            "${CMAKE_CURRENT_BINARY_DIR}/../mylib.dll"
        VERBATIM)
endif()

## Scopes:
#####################################################################

qt_internal_extend_target(mylib CONDITION MSVC
    DEFINES
        WIN32_MSVC
)
