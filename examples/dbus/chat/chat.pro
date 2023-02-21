QT += dbus widgets

HEADERS += chat.h
SOURCES += chat.cpp
FORMS += chatmainwindow.ui

DBUS_ADAPTORS += org.example.chat.xml
DBUS_INTERFACES += org.example.chat.xml

CONFIG += no_batch  # work around QTBUG-96513

# install
target.path = $$[QT_INSTALL_EXAMPLES]/dbus/chat
INSTALLS += target
