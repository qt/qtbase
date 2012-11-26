HEADERS     = dragwidget.h
RESOURCES   = draggableicons.qrc
SOURCES     = dragwidget.cpp \
              main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/draganddrop/draggableicons
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/draganddrop/draggableicons
INSTALLS += target sources

QT += widgets
