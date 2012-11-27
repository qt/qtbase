QT += gui

SOURCES += main.cpp
SOURCES += glwindow.cpp

HEADERS += glwindow.h

RESOURCES += texture.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/opengl/hellogl_es
INSTALLS += target
