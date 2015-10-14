QT += widgets

HEADERS     = dragwidget.h
RESOURCES   = draggabletext.qrc
SOURCES     = dragwidget.cpp \
              main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/draganddrop/draggabletext
INSTALLS += target
