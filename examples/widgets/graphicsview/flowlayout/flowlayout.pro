QT += widgets

QMAKE_PROJECT_NAME = flowlayout_graphicsview

HEADERS += flowlayout.h window.h
SOURCES += flowlayout.cpp main.cpp window.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/graphicsview/flowlayout
INSTALLS += target
