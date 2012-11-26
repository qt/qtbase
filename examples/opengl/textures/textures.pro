HEADERS       = glwidget.h \
                window.h
SOURCES       = glwidget.cpp \
                main.cpp \
                window.cpp
RESOURCES     = textures.qrc
QT           += opengl widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/textures
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS textures.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/opengl/textures
INSTALLS += target sources


simulator: warning(This example might not fully work on Simulator platform)
