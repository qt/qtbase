option(host_build)

CONFIG += force_bootstrap
SOURCES += gen_qfloat16_tables.cpp

load(qt_tool)

lib.CONFIG = dummy_install
INSTALLS = lib
