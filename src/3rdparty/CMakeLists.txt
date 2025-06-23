# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
if(QT_FEATURE_gui AND QT_FEATURE_png AND NOT QT_FEATURE_system_png)
    add_subdirectory(libpng)
endif()
qt_install_3rdparty_library_wrap_config_extra_file(BundledLibpng)

if(QT_FEATURE_gui AND QT_FEATURE_jpeg AND NOT QT_FEATURE_system_jpeg)
    add_subdirectory(libjpeg)
endif()
qt_install_3rdparty_library_wrap_config_extra_file(BundledLibjpeg)

# Needed for freetype, because it links against WrapPNG
qt_find_package(WrapPNG MODULE PROVIDED_TARGETS WrapPNG::WrapPNG)
if(QT_FEATURE_gui AND QT_FEATURE_freetype AND NOT QT_FEATURE_system_freetype)
    add_subdirectory(freetype)
endif()
qt_install_3rdparty_library_wrap_config_extra_file(BundledFreetype)

if(QT_FEATURE_gui AND QT_FEATURE_harfbuzz AND NOT QT_FEATURE_system_harfbuzz)
    add_subdirectory(harfbuzz-ng)
endif()
qt_install_3rdparty_library_wrap_config_extra_file(BundledHarfbuzz)

if (ANDROID)
    add_subdirectory(gradle)
endif()
