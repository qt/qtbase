# Generated from listnames.pro.

#####################################################################
## listnames Binary:
#####################################################################

add_qt_executable(listnames
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/dbus/listnames"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/dbus/listnames"
    SOURCES
        listnames.cpp
    LIBRARIES
        # Remove: gui
        Qt::DBus
)

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:listnames.pro:WIN32:
# CONFIG = "console"
