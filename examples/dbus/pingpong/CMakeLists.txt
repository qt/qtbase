# Generated from pingpong.pro.

#####################################################################
## ping Binary:
#####################################################################

add_qt_executable(ping
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/dbus/pingpong"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/dbus/pingpong"
    SOURCES
        ping.cpp
        ping-common.h
    LIBRARIES
        # Remove: gui
        Qt::DBus
)
#####################################################################
## pong Binary:
#####################################################################

add_qt_executable(pong
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/dbus/pingpong"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/dbus/pingpong"
    SOURCES
        ping-common.h
        pong.cpp pong.h
    LIBRARIES
        # Remove: gui
        Qt::DBus
)

if(WIN32)
endif()
