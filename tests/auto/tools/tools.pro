TEMPLATE=subdirs
SUBDIRS=\
   qmake \
   uic \
   moc \
   rcc \

contains(QT_CONFIG, dbus):SUBDIRS += qdbuscpp2xml qdbusxml2cpp
