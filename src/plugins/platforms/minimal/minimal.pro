TARGET = qminimal

QT += \
    core-private gui-private

!win32: QT += eventdispatcher_support-private
!darwin: QT += fontdatabase_support-private

DEFINES += QT_NO_FOREACH

SOURCES =   main.cpp \
            qminimalintegration.cpp \
            qminimalbackingstore.cpp
HEADERS =   qminimalintegration.h \
            qminimalbackingstore.h

OTHER_FILES += minimal.json

qtConfig(freetype): QMAKE_USE_PRIVATE += freetype

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QMinimalIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)
