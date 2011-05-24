TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .
QT -= gui
CONFIG += qdbus

# Input
HEADERS += complexpong.h
SOURCES += complexpong.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dbus/complexpingpong
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dbus/complexpingpong
INSTALLS += target sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example
symbian: warning(This example does not work on Symbian platform)
simulator: warning(This example does not work on Simulator platform)
