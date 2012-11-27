QT += dbus widgets

HEADERS += chat.h
SOURCES += chat.cpp
FORMS += chatmainwindow.ui chatsetnickname.ui

DBUS_ADAPTORS += org.example.chat.xml
DBUS_INTERFACES += org.example.chat.xml

# install
target.path = $$[QT_INSTALL_EXAMPLES]/dbus/chat
INSTALLS += target

simulator: warning(This example does not work on Simulator platform)
