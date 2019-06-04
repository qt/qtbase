# Generated from corelib.pro.

add_subdirectory(ipc)
add_subdirectory(mimetypes)
add_subdirectory(serialization)
add_subdirectory(tools)

if(QT_FEATURE_thread)
    add_subdirectory(threads)
endif()
