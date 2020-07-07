# Generated from dbus.pro.

if(NOT TARGET Qt::DBus)
    return()
endif()
add_subdirectory(listnames)
add_subdirectory(pingpong)
if(QT_FEATURE_process)
    add_subdirectory(complexpingpong)
endif()
if(TARGET Qt::Widgets)
    add_subdirectory(chat)
    add_subdirectory(remotecontrolledcar)
endif()
