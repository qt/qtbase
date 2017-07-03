SOURCES += main.cpp
QT -= gui
CONFIG -= app_bundle

RESOURCES   = resources.qrc

win32: CONFIG += console

# install
target.path = $$[QT_INSTALL_EXAMPLES]/xml/htmlinfo
INSTALLS += target
