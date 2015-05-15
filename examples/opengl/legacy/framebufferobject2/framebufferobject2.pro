HEADERS += glwidget.h
SOURCES += glwidget.cpp main.cpp
RESOURCES += framebufferobject2.qrc

QT += opengl widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/legacy/framebufferobject2
INSTALLS += target

contains(QT_CONFIG, opengles.|angle|dynamicgl):error("This example requires Qt to be configured with -opengl desktop")
