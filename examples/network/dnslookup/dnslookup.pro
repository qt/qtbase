TEMPLATE = app
QT = core network
CONFIG += cmdline
HEADERS += dnslookup.h
SOURCES += dnslookup.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/dnslookup
INSTALLS += target
