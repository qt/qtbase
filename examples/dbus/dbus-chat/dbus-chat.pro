QT += dbus widgets

HEADERS += chat.h
SOURCES += chat.cpp
FORMS += chatmainwindow.ui chatsetnickname.ui

DBUS_ADAPTORS += org.example.chat.xml
DBUS_INTERFACES += org.example.chat.xml

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dbus/chat
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro *.xml
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dbus/dbus-chat
INSTALLS += target sources

simulator: warning(This example does not work on Simulator platform)
