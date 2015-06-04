HEADERS       = glwidget.h \
                mainwindow.h
SOURCES       = glwidget.cpp \
                main.cpp \
                mainwindow.cpp
QT           += opengl widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/legacy/grabber
INSTALLS += target

contains(QT_CONFIG, opengles.|angle|dynamicgl):error("This example requires Qt to be configured with -opengl desktop")
