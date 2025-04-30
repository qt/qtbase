# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Need an explicit call at the top level. This is the absolute minimum version
# needed to configure the project with any combination of enabled features.
# The call to qt_build_repo_begin() will upgrade policies further.
#
# The absolute minimum version for building Qt is 3.16 because for metatype.json generation, we
# depend on being able to find the location of json files created by AUTOMOC/moc.
cmake_minimum_required(VERSION 3.16)

# Get the repo version and CMake policy details
include(.cmake.conf)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/QtBaseHelpers.cmake)

qt_internal_check_if_path_has_symlinks("${CMAKE_BINARY_DIR}")

# This needs to be called before the first project() call.
qt_internal_qtbase_run_autodetect()

# This call will load any provided cmake toolchain file.
project(QtBase
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt Base Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

if(UNIX AND NOT ANDROID)
  enable_language(ASM)
endif()

set(QT_BUILD_EXTRA_IDE_FILE_PATTERNS bin/* libexec/*)

qt_internal_qtbase_build_repo()
