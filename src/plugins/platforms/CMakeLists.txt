if(QT_FEATURE_xcb)
    add_subdirectory(xcb)
endif()

if(APPLE)
    add_subdirectory(cocoa)
endif(APPLE)

