QT          += opengl

HEADERS     = glwidget.h \
              helper.h \
              widget.h \
              window.h
SOURCES     = glwidget.cpp \
              helper.cpp \
              main.cpp \
              widget.cpp \
              window.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/opengl/2dpainting
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS 2dpainting.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/opengl/2dpainting
INSTALLS += target sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
simulator: warning(This example might not fully work on Simulator platform)
