HEADERS     = dragwidget.h
RESOURCES   = draggableicons.qrc
SOURCES     = dragwidget.cpp \
              main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/draganddrop/draggableicons
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/draganddrop/draggableicons
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000C615
    include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
}
