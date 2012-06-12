TARGET = cupsprintersupport
load(qt_plugin)
DESTDIR = $$QT.gui.plugins/printsupport

QT += core-private gui-private printsupport printsupport-private

INCLUDEPATH += ../../../printsupport/kernel

SOURCES += main.cpp \
    qcupsprintersupport.cpp \
    qcupsprintengine.cpp

HEADERS += qcupsprintersupport_p.h \
    qcupsprintengine_p.h

OTHER_FILES += cups.json

target.path += $$[QT_INSTALL_PLUGINS]/printsupport
INSTALLS += target
