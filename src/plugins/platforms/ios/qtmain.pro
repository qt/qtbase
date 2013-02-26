TARGET = qiosmain

PLUGIN_TYPE = platforms
load(qt_plugin)

QT += gui-private

OBJECTIVE_SOURCES = qtmain.mm \
    qiosviewcontroller.mm

HEADERS = qiosviewcontroller.h
