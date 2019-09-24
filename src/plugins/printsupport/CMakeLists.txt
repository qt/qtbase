if (APPLE_OSX)
    add_subdirectory(cocoa)
endif()

if (WIN32)
#     TODO add_subdirectory(windows)
endif()

if (UNIX AND NOT APPLE AND QT_FEATURE_cups)
    add_subdirectory(cups)
endif()
