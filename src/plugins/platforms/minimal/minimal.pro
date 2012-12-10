TARGET = qminimal

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QMinimalIntegrationPlugin
load(qt_plugin)

QT += core-private gui-private platformsupport-private

SOURCES =   main.cpp \
            qminimalintegration.cpp \
            qminimalbackingstore.cpp
HEADERS =   qminimalintegration.h \
            qminimalbackingstore.h

OTHER_FILES += minimal.json
