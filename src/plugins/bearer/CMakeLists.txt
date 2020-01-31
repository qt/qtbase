# Generated from bearer.pro.

# special case begin
# The whole block is manual.
if(NOT ANDROID AND LINUX AND TARGET Qt::DBus)
    add_subdirectory(generic)
    add_subdirectory(connman)
    add_subdirectory(networkmanager)
elseif(ANDROID AND NOT ANDROID_EMBEDDED)
    add_subdirectory(android)
else()
    add_subdirectory(generic)
endif()
# special case end
