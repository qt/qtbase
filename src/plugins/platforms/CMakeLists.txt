if (NOT ANDROID)
    add_subdirectory(minimal)
endif()

if(QT_FEATURE_xcb)
    add_subdirectory(xcb)
endif()

if(QT_FEATURE_linuxfb)
    add_subdirectory(linuxfb)
endif()

if(APPLE)
    add_subdirectory(cocoa)
endif(APPLE)

