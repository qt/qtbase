# Generated from bearermonitor.pro.

#####################################################################
## bearermonitor Binary:
#####################################################################

add_qt_executable(bearermonitor
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/bearermonitor"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/bearermonitor"
    SOURCES
        bearermonitor.cpp bearermonitor.h
        bearermonitor_240_320.ui
        bearermonitor_640_480.ui
        main.cpp
        sessionwidget.cpp sessionwidget.h sessionwidget.ui
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Network
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

#### Keys ignored in scope 1:.:.:bearermonitor.pro:<TRUE>:
# CONFIG = "console"

## Scopes:
#####################################################################

extend_target(bearermonitor CONDITION WIN32
    PUBLIC_LIBRARIES
        ws2_32
)
