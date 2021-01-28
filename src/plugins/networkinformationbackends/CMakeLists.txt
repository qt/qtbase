if(WIN32 AND QT_FEATURE_networklistmanager)
    add_subdirectory(networklistmanager)
endif()

if(LINUX)
    add_subdirectory(networkmanager)
endif()

if(APPLE)
    add_subdirectory(scnetworkreachability)
endif()
