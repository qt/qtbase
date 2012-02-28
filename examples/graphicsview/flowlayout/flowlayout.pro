QT += widgets

QMAKE_PROJECT_NAME = flowlayout_graphicsview

HEADERS += flowlayout.h window.h
SOURCES += flowlayout.cpp main.cpp window.cpp

simulator: warning(This example might not fully work on Simulator platform)
