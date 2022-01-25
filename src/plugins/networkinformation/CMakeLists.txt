if(WIN32 AND QT_FEATURE_networklistmanager)
    add_subdirectory(networklistmanager)
endif()

if(LINUX AND TARGET Qt::DBus)
    add_subdirectory(networkmanager)
endif()

if(APPLE)
    add_subdirectory(scnetworkreachability)
endif()

if(ANDROID)
    add_subdirectory(android)
endif()

if(QT_FEATURE_glib AND TARGET GLIB2::GOBJECT AND TARGET GLIB2::GIO)
    add_subdirectory(glib)
endif()
