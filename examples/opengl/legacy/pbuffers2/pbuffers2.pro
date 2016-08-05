QT += opengl svg widgets

HEADERS += glwidget.h
SOURCES += glwidget.cpp main.cpp
RESOURCES += pbuffers2.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/legacy/pbuffers2
INSTALLS += target

qtConfig(opengles.|angle|dynamicgl): error("This example requires Qt to be configured with -opengl desktop")
