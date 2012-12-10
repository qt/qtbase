TARGET = cupsprintersupport
MODULE = cupsprintersupport
PLUGIN_TYPE = printsupport
PLUGIN_CLASS_NAME = QCupsPrinterSupportPlugin
load(qt_plugin)

QT += core-private gui-private printsupport printsupport-private

INCLUDEPATH += ../../../printsupport/kernel

SOURCES += main.cpp \
    qcupsprintersupport.cpp \
    qcupsprintengine.cpp

HEADERS += qcupsprintersupport_p.h \
    qcupsprintengine_p.h

OTHER_FILES += cups.json
