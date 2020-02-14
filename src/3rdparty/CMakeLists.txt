# special case skip regeneration
# The file is maintained manually
add_subdirectory(harfbuzz)
add_subdirectory(double-conversion)

if(QT_FEATURE_png AND NOT QT_FEATURE_system_png)
    add_subdirectory(libpng)
endif()

if(QT_FEATURE_freetype AND NOT QT_FEATURE_system_freetype)
    add_subdirectory(freetype)
endif()

if(QT_FEATURE_harfbuzz AND NOT QT_FEATURE_system_harfbuzz)
    add_subdirectory(harfbuzz-ng)
endif()

if(QT_FEATURE_regularexpression AND NOT QT_FEATURE_system_pcre2)
    add_subdirectory(pcre2)
endif()

if (ANDROID)
    add_subdirectory(gradle)
endif()
