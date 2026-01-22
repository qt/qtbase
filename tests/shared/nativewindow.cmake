# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include_guard(GLOBAL)

add_library(nativewindow INTERFACE)
target_include_directories(nativewindow INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_sources(nativewindow INTERFACE ${CMAKE_CURRENT_LIST_DIR}/nativewindow.h)
target_link_libraries(nativewindow INTERFACE Qt::Core)

if(APPLE OR WIN32 OR QT_FEATURE_xcb OR ANDROID)
    target_compile_definitions(nativewindow INTERFACE HAVE_NATIVE_WINDOW)
else()
    return()
endif()

target_sources(nativewindow INTERFACE ${CMAKE_CURRENT_LIST_DIR}/nativewindow.cpp)

if(QT_FEATURE_xcb)
    target_link_libraries(nativewindow INTERFACE XCB::XCB)
    target_link_libraries(nativewindow INTERFACE XCB::ICCCM)
endif()

if(APPLE)
    target_sources(nativewindow INTERFACE ${CMAKE_CURRENT_LIST_DIR}/nativewindow.mm)
endif()
