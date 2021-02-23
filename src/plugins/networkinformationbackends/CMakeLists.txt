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
