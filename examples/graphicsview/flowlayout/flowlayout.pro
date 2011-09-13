TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

QMAKE_PROJECT_NAME = flowlayout_graphicsview

# Input
HEADERS += flowlayout.h window.h
SOURCES += flowlayout.cpp main.cpp window.cpp
QT += widgets

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example
simulator: warning(This example might not fully work on Simulator platform)
