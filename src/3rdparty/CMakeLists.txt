if(QT_FEATURE_gui AND QT_FEATURE_png AND NOT QT_FEATURE_system_png)
    add_subdirectory(libpng)
endif()
qt_install_3rdparty_library_wrap_config_extra_file(BundledLibpng)

if(QT_FEATURE_gui AND QT_FEATURE_jpeg AND NOT QT_FEATURE_system_jpeg)
    add_subdirectory(libjpeg)
endif()
qt_install_3rdparty_library_wrap_config_extra_file(BundledLibjpeg)

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
