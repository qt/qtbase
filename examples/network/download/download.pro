QT = core network
CONFIG += cmdline

SOURCES += main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/download
INSTALLS += target
