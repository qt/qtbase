# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## TestInternal Module:
#####################################################################

# file properties must be set before calling `qt_internal_add_module` until it can be `DEFER`ed
set_source_files_properties(
    3rdparty/cmake/QtRunCMakeTestHelpers.cmake
    3rdparty/cmake/RunCMake.cmake
    PROPERTIES
        QT_INSTALL_PATH 3rdparty/cmake
)

qt_internal_add_module(TestInternalsPrivate
    # Dummy interface target
    INTERNAL_MODULE
    HEADER_MODULE
    NO_GENERATE_CPP_EXPORTS
    NO_MODULE_HEADERS
    NO_ADDITIONAL_TARGET_INFO

    ATTRIBUTION_FILE_DIR_PATHS
        3rdparty/cmake
    EXTRA_CMAKE_FILES
        3rdparty/cmake/QtRunCMakeTestHelpers.cmake
        3rdparty/cmake/RunCMake.cmake
    EXTRA_CMAKE_INCLUDES
        3rdparty/cmake/QtRunCMakeTestHelpers.cmake
)
