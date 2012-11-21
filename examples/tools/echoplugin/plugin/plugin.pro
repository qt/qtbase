#! [0]
TEMPLATE        = lib
CONFIG         += plugin
INCLUDEPATH    += ../echowindow
HEADERS         = echoplugin.h
SOURCES         = echoplugin.cpp
OTHER_FILES    += echoplugin.json
TARGET          = $$qtLibraryTarget(echoplugin)
DESTDIR         = ../plugins
#! [0]

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/echoplugin/plugin
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS plugin.pro echoplugin.json
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/echoplugin/plugin
INSTALLS += target sources

QT += widgets
