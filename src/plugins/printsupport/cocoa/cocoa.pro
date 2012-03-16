TARGET = cocoaprintersupport
load(qt_plugin)
DESTDIR = $$QT.gui.plugins/printsupport

QT += printsupport
LIBS += -framework Cocoa

SOURCES += main.cpp

OTHER_FILES += cocoa.json

target.path += $$[QT_INSTALL_PLUGINS]/printsupport
INSTALLS += target
