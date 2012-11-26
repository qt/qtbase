HEADERS += glwidget.h
SOURCES += glwidget.cpp main.cpp
RESOURCES += framebufferobject2.qrc

QT += opengl widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/framebufferobject2
sources.files = $$SOURCES $$HEADERS $$RESOURCES framebufferobject2.pro *.png
sources.path = $$[QT_INSTALL_EXAMPLES]/opengl/framebufferobject2
INSTALLS += target sources


simulator: warning(This example might not fully work on Simulator platform)
