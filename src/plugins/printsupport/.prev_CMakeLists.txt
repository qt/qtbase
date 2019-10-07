# Generated from printsupport.pro.

if(APPLE_OSX)
    add_subdirectory(cocoa)
endif()
if(WIN32)
    add_subdirectory(windows)
endif()
if(QT_FEATURE_cups AND UNIX AND NOT APPLE)
    add_subdirectory(cups)
endif()
