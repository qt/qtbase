HEADERS += glwidget.h
SOURCES += glwidget.cpp main.cpp

QT += opengl

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/opengl/samplebuffers
sources.files = $$SOURCES $$HEADERS samplebuffers.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/opengl/samplebuffers
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
