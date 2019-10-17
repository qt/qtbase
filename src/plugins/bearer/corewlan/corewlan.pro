TARGET = qcorewlanbearer

QT = core-private network-private
LIBS += -framework Foundation -framework SystemConfiguration

qtConfig(corewlan) {
    LIBS += -framework CoreWLAN -framework Security
}

HEADERS += qcorewlanengine.h

SOURCES += main.cpp

OBJECTIVE_SOURCES += qcorewlanengine.mm

OTHER_FILES += corewlan.json

PLUGIN_TYPE = bearer
PLUGIN_CLASS_NAME = QCoreWlanEnginePlugin
load(qt_plugin)
