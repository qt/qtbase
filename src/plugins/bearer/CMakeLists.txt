# Generated from bearer.pro.

if(NOT ANDROID AND LINUX AND TARGET Qt::DBus) # special case
    add_subdirectory(generic)
    add_subdirectory(connman)
    add_subdirectory(networkmanager)
elseif(ANDROID AND NOT ANDROID_EMBEDDED) # special case
    add_subdirectory(android)
else() # special case
    add_subdirectory(generic)
endif()
