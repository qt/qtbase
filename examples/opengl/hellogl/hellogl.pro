VPATH += ../shared
INCLUDEPATH += ../shared

HEADERS       = glwidget.h \
                window.h \
                qtlogo.h
SOURCES       = glwidget.cpp \
                main.cpp \
                window.cpp \
                qtlogo.cpp
QT           += opengl

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/opengl/hellogl
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS hellogl.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/opengl/hellogl
INSTALLS += target sources

symbian: CONFIG += qt_example
