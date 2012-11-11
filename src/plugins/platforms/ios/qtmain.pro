TARGET = qiosmain

load(qt_plugin)

OBJECTIVE_SOURCES = qtmain.mm

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target
