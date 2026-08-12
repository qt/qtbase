# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdrag Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qdrag LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qdrag
    SOURCES
        tst_qdrag.cpp
    LIBRARIES
        Qt::GuiPrivate
)

# On most platforms, QDrag::exec() runs a nested event loop that only a real
# user can terminate. The offscreen plugin uses the in-process QSimpleDrag and
# implements QCursor::setPos() synchronously, so synthetic input works
# deterministically. The tests that need this QSKIP on other platforms.
set_property(TEST tst_qdrag APPEND PROPERTY ENVIRONMENT
    "QT_QPA_PLATFORM=offscreen"
)

# Prioritize building the offscreen plugin before tst_qdrag, if possible.
if(TARGET QOffscreenIntegrationPlugin)
    add_dependencies(tst_qdrag QOffscreenIntegrationPlugin)
endif()
