HEADERS += glwidget.h
SOURCES += glwidget.cpp main.cpp

QT += opengl widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/legacy/samplebuffers
INSTALLS += target

qtConfig(opengles.|angle|dynamicgl): error("This example requires Qt to be configured with -opengl desktop")
