# Generated from ipc.pro.

if(NOT TARGET Qt::Widgets)
    return()
endif()
if(QT_FEATURE_sharedmemory)
    qt_internal_add_example(sharedmemory)
endif()
if(QT_FEATURE_localserver AND TARGET Qt::Network)
    qt_internal_add_example(localfortuneserver)
    qt_internal_add_example(localfortuneclient)
endif()
