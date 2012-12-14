QT += widgets

HEADERS   = layoutitem.h \
            window.h
SOURCES   = layoutitem.cpp \
            main.cpp \
            window.cpp
RESOURCES = basicgraphicslayouts.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/graphicsview/basicgraphicslayouts
INSTALLS += target
