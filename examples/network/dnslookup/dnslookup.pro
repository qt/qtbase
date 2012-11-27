TEMPLATE = app
QT = core network
mac:CONFIG -= app_bundle
win32:CONFIG += console
HEADERS += dnslookup.h
SOURCES += dnslookup.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/dnslookup
INSTALLS += target
