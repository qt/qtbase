TEMPLATE = app
TARGET = mapdemo
QT += concurrent
CONFIG += cmdline

SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtconcurrent/map
INSTALLS += target
