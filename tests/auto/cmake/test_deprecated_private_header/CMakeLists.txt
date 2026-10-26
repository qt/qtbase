# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

include(.cmake.conf)

project(TestDeprecatedPrivateHeader
        DESCRIPTION "Test of a deprecated header replaced by a private header"
        HOMEPAGE_URL "https://qt.io/"
        LANGUAGES CXX C
        VERSION "${QT_REPO_MODULE_VERSION}"
)

find_package(Qt6 COMPONENTS Core BuildInternals CONFIG REQUIRED)
qt_internal_project_setup()

qt_build_repo_begin()

qt_internal_add_module(TestDeprecatedPrivateHeader
    NO_UNITY_BUILD
    SOURCES
        module_api.h
        module_api.cpp
        replacement_p.h
    PUBLIC_LIBRARIES
        Qt::Core
)

qt_internal_extend_target(TestDeprecatedPrivateHeader CONDITION DEPRECATE_PRIVATE_HEADER
    SOURCES
        replaces_deprecated_p.h
)

add_executable(TestDeprecatedPrivateHeaderApp use_deprecated_header.cpp)
target_link_libraries(TestDeprecatedPrivateHeaderApp PRIVATE TestDeprecatedPrivateHeaderPrivate)
set_target_properties(TestDeprecatedPrivateHeaderApp PROPERTIES UNITY_BUILD OFF)

qt_build_repo_post_process()
qt_build_repo_end()
