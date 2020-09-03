# special case skip regeneration
# The file is maintained manually

if(QT_FEATURE_gui AND QT_FEATURE_png AND NOT QT_FEATURE_system_png)
    add_subdirectory(libpng)
endif()
qt_install_3rdparty_library_wrap_config_extra_file(BundledLibpng)

if(QT_FEATURE_gui AND QT_FEATURE_freetype AND NOT QT_FEATURE_system_freetype)
    add_subdirectory(freetype)
endif()
qt_install_3rdparty_library_wrap_config_extra_file(BundledFreetype)

if(QT_FEATURE_gui AND QT_FEATURE_harfbuzz AND NOT QT_FEATURE_system_harfbuzz)
    add_subdirectory(harfbuzz-ng)
endif()
qt_install_3rdparty_library_wrap_config_extra_file(BundledHarfbuzz)

if(QT_FEATURE_regularexpression AND NOT QT_FEATURE_system_pcre2)
    add_subdirectory(pcre2)
endif()
qt_install_3rdparty_library_wrap_config_extra_file(BundledPcre2)

if (ANDROID)
    add_subdirectory(gradle)
endif()
