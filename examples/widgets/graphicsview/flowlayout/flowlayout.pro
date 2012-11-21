QT += widgets

QMAKE_PROJECT_NAME = flowlayout_graphicsview

HEADERS += flowlayout.h window.h
SOURCES += flowlayout.cpp main.cpp window.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/graphicsview/flowlayout
sources.files = $$SOURCES $$HEADERS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/graphicsview/flowlayout
INSTALLS += target sources

simulator: warning(This example might not fully work on Simulator platform)
