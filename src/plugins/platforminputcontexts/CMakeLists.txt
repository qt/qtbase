
if(QT_FEATURE_dbus AND (NOT APPLE_OSX) AND (NOT WIN32) AND (NOT EMSCRIPTEN))
    add_subdirectory(ibus)
endif()

if(QT_FEATURE_xcb)
    add_subdirectory(compose)
endif()


