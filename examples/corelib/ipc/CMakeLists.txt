# Generated from ipc.pro.


if(QT_FEATURE_sharedmemory)
    add_subdirectory(sharedmemory)
endif()

if(TARGET Qt::Network)

    if(QT_FEATURE_localserver)
        add_subdirectory(localfortuneserver)
        add_subdirectory(localfortuneclient)
    endif()
endif()
