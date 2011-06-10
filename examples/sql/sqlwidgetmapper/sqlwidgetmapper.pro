HEADERS   = window.h
SOURCES   = main.cpp \
            window.cpp
QT += sql widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/sql/sqlwidgetmapper
sources.files = $$SOURCES $$HEADERS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/sql/sqlwidgetmapper
INSTALLS += target sources

wince*: DEPLOYMENT_PLUGIN += qsqlite

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example

