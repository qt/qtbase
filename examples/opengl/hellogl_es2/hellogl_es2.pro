QT += opengl widgets

SOURCES += main.cpp
SOURCES += glwidget.cpp
SOURCES += mainwindow.cpp
SOURCES += bubble.cpp

HEADERS += glwidget.h
HEADERS += mainwindow.h
HEADERS += bubble.h

RESOURCES += texture.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/hellogl_es2
INSTALLS += target

maemo5 {
    # Debian package name may not contain numbers or special characters
    # such as '_', lets change this in Maemo.
    TARGET = helloglestwo
    CONFIG += qt_example
}

simulator: warning(This example might not fully work on Simulator platform)
