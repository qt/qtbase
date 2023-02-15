# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## mylib Generic Library:
#####################################################################

qt_internal_add_cmake_library(mylib2
    SHARED
    INSTALL_DIRECTORY "${INSTALL_TESTSDIR}tst_qlibrary"
    #OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    SOURCES
        mylib.c
    LIBRARIES
        Qt::Core
)

# This test is very "annoying" to get working with CMake as it involves having
# two targets with the same name on the parent scope, which is not possible with
# CMake. Furthermore, on UNIX, this version of the library should override the
# root symlink (libmylib.so) to point to version 2.
# Since we can't build two targets with the same name and in the same directory,
# we build mylib2 in it's own directory and manually copy and create the
# symlinks in the parent directory.
# Finally we also need to create a libmylib.so2 file in the parent directory.
#
set_target_properties(mylib2 PROPERTIES
    OUTPUT_NAME mylib
)
set_target_properties(mylib2 PROPERTIES
    VERSION 2.0.0
    SOVERSION 2
    C_VISIBILITY_PRESET "default"
    CXX_VISIBILITY_PRESET "default"
)

if(WIN32)
    # CMake sets for Windows-GNU platforms the suffix "lib"
    set_property(TARGET mylib2 PROPERTY PREFIX "")
endif()

if(UNIX)
    if(APPLE)
        add_custom_command(TARGET mylib2 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:mylib2>
                "${CMAKE_CURRENT_BINARY_DIR}/../system.qt.test.mylib.so.dylib"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:mylib2>
                "${CMAKE_CURRENT_BINARY_DIR}/../libmylib.2.0.0.dylib"
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                "libmylib.2.0.0.dylib"
                "${CMAKE_CURRENT_BINARY_DIR}/../libmylib.2.dylib"
            COMMAND ${CMAKE_COMMAND} -E remove
                "${CMAKE_CURRENT_BINARY_DIR}/../libmylib.dylib"
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                "libmylib.2.0.0.dylib"
                "${CMAKE_CURRENT_BINARY_DIR}/../libmylib.dylib"
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                "libmylib.2.0.0.dylib"
                "${CMAKE_CURRENT_BINARY_DIR}/../libmylib.so2.dylib"
            VERBATIM)
    elseif(NOT ANDROID)
        add_custom_command(TARGET mylib2 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:mylib2>
                "${CMAKE_CURRENT_BINARY_DIR}/../system.qt.test.mylib.so"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:mylib2>
                "${CMAKE_CURRENT_BINARY_DIR}/../libmylib.so.2.0.0"
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                "libmylib.so.2.0.0"
                "${CMAKE_CURRENT_BINARY_DIR}/../libmylib.so.2"
            COMMAND ${CMAKE_COMMAND} -E remove
                "${CMAKE_CURRENT_BINARY_DIR}/../libmylib.so"
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                "libmylib.so.2.0.0"
                "${CMAKE_CURRENT_BINARY_DIR}/../libmylib.so"
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                "libmylib.so.2.0.0"
                "${CMAKE_CURRENT_BINARY_DIR}/../libmylib.so2"
            VERBATIM)
    else()
        # Android does not use symlinks. Also, according to our conventions,
        # libraries on Android MUST be named in the following pattern:
        # lib*.so
        add_custom_command(TARGET mylib2 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:mylib2>
            "${CMAKE_CURRENT_BINARY_DIR}/../libsystem.qt.test.mylib.so"
            VERBATIM)
    endif()
else() #Win32
    add_custom_command(TARGET mylib2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:mylib2>
            "${CMAKE_CURRENT_BINARY_DIR}/../system.qt.test.mylib.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:mylib2>
            "${CMAKE_CURRENT_BINARY_DIR}/../mylib.dl2"
        VERBATIM)
endif()

## Scopes:
#####################################################################

qt_internal_extend_target(mylib2 CONDITION MSVC
    DEFINES
        WIN32_MSVC
)
