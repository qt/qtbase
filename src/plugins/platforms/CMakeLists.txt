if (NOT ANDROID)
    add_subdirectory(minimal)
endif()

if (NOT ANDROID AND QT_FEATURE_freetype)
    add_subdirectory(offscreen)
endif()

if(QT_FEATURE_xcb)
    add_subdirectory(xcb)
endif()

if(QT_FEATURE_eglfs)
    add_subdirectory(minimalegl)
endif()

if(QT_FEATURE_linuxfb)
    add_subdirectory(linuxfb)
endif()

if(APPLE)
    add_subdirectory(cocoa)
endif(APPLE)

