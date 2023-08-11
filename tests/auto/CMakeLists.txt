# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Order by dependency [*], then alphabetic. [*] If bugs in part A of
# our source would break tests of part B, then test A before B.

set(run_dbus_tests OFF)
if (QT_FEATURE_dbus)
    set(run_dbus_tests ON)
    if(NOT CMAKE_CROSSCOMPILING AND TARGET Qt::DBus)
        execute_process(COMMAND dbus-send --session --type=signal / local.AutotestCheck.Hello
            TIMEOUT 5
            RESULT_VARIABLE dbus_session_test
            OUTPUT_QUIET ERROR_QUIET)
        if(NOT "${dbus_session_test}" STREQUAL "0")
            set(run_dbus_tests OFF)
            if(QT_FEATURE_dbus_linked)
                message(WARNING
                    " QtDBus is enabled but session bus is not available for testing.\n"
                    " Please check the installation. Skipping QtDBus tests.")
            else()
                message(WARNING
                    " QtDBus is enabled with runtime support, but session bus is not available.\n"
                    " Skipping QtDBus tests.")
            endif()
        endif()
    endif()
endif()

if(UIKIT)
    # For now, only build CMake auto tests when QT_BUILD_MINIMAL_STATIC_TEST
    # is a requested (which is basically what CI will build).
    # The reason is that qt_internal_add_test doesn't use qt_add_executable
    # at the moment, which means none of the required iOS specific finalizers
    # are run.
    if(QT_BUILD_MINIMAL_STATIC_TESTS)
        add_subdirectory(cmake)
        return()
    endif()

    # Build only corelib and gui tests when targeting uikit (iOS),
    # because the script can't handle the SUBDIRS assignment well.
    add_subdirectory(corelib)
    if (TARGET Qt::Gui)
        add_subdirectory(gui)
    endif()
    return()
endif()

# Limit set of tests to run for Android multi-ABI Qt builds.
if(QT_BUILD_MINIMAL_ANDROID_MULTI_ABI_TESTS)
    add_subdirectory(cmake)
    add_subdirectory(corelib/kernel/qmath)
    add_subdirectory(widgets/effects/qpixmapfilter)
    add_subdirectory(corelib/platform)
    return()
endif()

# Only configure a single auto test for wasm for now
# Since the linking step at this point is prohibitively long (static linking)
if(WASM)
    add_subdirectory(corelib/text/qchar)
    return()
endif()

add_subdirectory(testlib)
if(NOT CMAKE_CROSSCOMPILING AND QT_FEATURE_process)
     add_subdirectory(tools)
endif()
add_subdirectory(corelib)
if (TARGET Qt::Xml AND TARGET Qt::Sql AND TARGET Qt::Network)
    add_subdirectory(cmake)
endif()

# Limit set of tests to run for static Qt builds.
if(QT_BUILD_MINIMAL_STATIC_TESTS)
    return()
endif()

if (TARGET Qt::Concurrent)
     add_subdirectory(concurrent)
endif()

if(run_dbus_tests)
    add_subdirectory(dbus)
endif()

if (TARGET Qt::Gui)
    add_subdirectory(gui)
endif()
if (TARGET Qt::Network)
     add_subdirectory(network)
endif()
if (TARGET Qt::OpenGL)
     add_subdirectory(opengl)
endif()
if (TARGET Qt::PrintSupport)
     add_subdirectory(printsupport)
endif()
if (TARGET Qt::Sql)
     add_subdirectory(sql)
endif()
if (TARGET Qt::Widgets)
     add_subdirectory(widgets)
endif()
if (TARGET Qt::Xml)
     add_subdirectory(xml)
endif()
# add_subdirectory(installed_cmake) ## FIXME: Does this still make sense in this form?
add_subdirectory(other)
