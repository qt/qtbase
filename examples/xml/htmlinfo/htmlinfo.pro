SOURCES += main.cpp
QT -= gui

RESOURCES   = resources.qrc

CONFIG += cmdline

# install
target.path = $$[QT_INSTALL_EXAMPLES]/xml/htmlinfo
INSTALLS += target
