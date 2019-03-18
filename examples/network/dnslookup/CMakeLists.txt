# Generated from dnslookup.pro.

#####################################################################
## dnslookup Binary:
#####################################################################

add_qt_executable(dnslookup
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/dnslookup"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/dnslookup"
    SOURCES
        dnslookup.cpp dnslookup.h
    LIBRARIES
        Qt::Network
)

#### Keys ignored in scope 1:.:dnslookup.pro:<NONE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:dnslookup.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

#### Keys ignored in scope 3:.:dnslookup.pro:WIN32:
# CONFIG = "console"
