TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

QMAKE_PROJECT_NAME = flowlayout_graphicsview

# Input
HEADERS += flowlayout.h window.h
SOURCES += flowlayout.cpp main.cpp window.cpp

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
maemo5: include($$QT_SOURCE_TREE/examples/maemo5pkgrules.pri)
simulator: warning(This example might not fully work on Simulator platform)
