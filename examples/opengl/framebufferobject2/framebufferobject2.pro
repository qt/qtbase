HEADERS += glwidget.h
SOURCES += glwidget.cpp main.cpp
RESOURCES += framebufferobject2.qrc

QT += opengl

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/opengl/framebufferobject2
sources.files = $$SOURCES $$HEADERS $$RESOURCES framebufferobject2.pro *.png
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/opengl/framebufferobject2
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
