HEADERS   = layoutitem.h \
            window.h
SOURCES   = layoutitem.cpp \
            main.cpp \
            window.cpp
RESOURCES = basicgraphicslayouts.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/graphicsview/basicgraphicslayouts
sources.files = $$SOURCES $$HEADERS $$RESOURCES basicgraphicslayouts.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/graphicsview/basicgraphicslayouts
INSTALLS += target sources

QT += widgets
