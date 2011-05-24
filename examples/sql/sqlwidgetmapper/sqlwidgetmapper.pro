HEADERS   = window.h
SOURCES   = main.cpp \
            window.cpp
QT += sql

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/sql/sqlwidgetmapper
sources.files = $$SOURCES $$HEADERS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/sql/sqlwidgetmapper
INSTALLS += target sources

wince*: DEPLOYMENT_PLUGIN += qsqlite

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
maemo5: include($$QT_SOURCE_TREE/examples/maemo5pkgrules.pri)

