# Generated from ipc.pro.

if(NOT TARGET Qt::Widgets)
    return()
endif()
if(QT_FEATURE_sharedmemory)
    add_subdirectory(sharedmemory)
endif()
if(QT_FEATURE_localserver AND TARGET Qt::Network)
    add_subdirectory(localfortuneserver)
    add_subdirectory(localfortuneclient)
endif()
