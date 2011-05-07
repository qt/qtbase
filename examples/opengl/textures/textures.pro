HEADERS       = glwidget.h \
                window.h
SOURCES       = glwidget.cpp \
                main.cpp \
                window.cpp
RESOURCES     = textures.qrc
QT           += opengl widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/opengl/textures
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS textures.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/opengl/textures
INSTALLS += target sources

symbian: CONFIG += qt_example
