QT  = core core-private sql-private

# For QMAKE_USE in the parent projects.
include($$shadowed($$PWD)/qtsqldrivers-config.pri)

PLUGIN_TYPE = sqldrivers
load(qt_plugin)

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII
