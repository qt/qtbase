HEADERS       = glwidget.h \
                mainwindow.h
SOURCES       = glwidget.cpp \
                main.cpp \
                mainwindow.cpp
QT           += opengl

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/opengl/grabber
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS grabber.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/opengl/grabber
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
