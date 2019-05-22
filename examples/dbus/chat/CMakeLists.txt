# Generated from chat.pro.

#####################################################################
## chat Binary:
#####################################################################

add_qt_executable(chat
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/dbus/chat"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/dbus/chat"
    SOURCES
        chat.cpp chat.h
        chatmainwindow.ui
        chatsetnickname.ui
    DBUS_ADAPTOR_SOURCES
        org.example.chat.xml
    DBUS_INTERFACE_SOURCES
        org.example.chat.xml
    PUBLIC_LIBRARIES
        Qt::DBus
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)
