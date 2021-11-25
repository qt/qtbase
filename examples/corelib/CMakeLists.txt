# Generated from corelib.pro.

add_subdirectory(bindableproperties)
add_subdirectory(ipc)
add_subdirectory(mimetypes)
add_subdirectory(serialization)
add_subdirectory(tools)
add_subdirectory(platform)
if(QT_FEATURE_thread)
    add_subdirectory(threads)
endif()
