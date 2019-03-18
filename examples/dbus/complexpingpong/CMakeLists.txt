# Generated from complexpong.pro.

#####################################################################
## complexpong Binary:
#####################################################################

add_qt_executable(complexpong
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/dbus/complexpingpong"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/dbus/complexpingpong"
    SOURCES
        complexpong.cpp complexpong.h
    LIBRARIES
        # Remove: gui
        Qt::DBus
)
