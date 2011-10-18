#-------------------------------------------------
#
# Project created by QtCreator 2010-06-23T12:55:35
#
#-------------------------------------------------

QT       += core gui widgets

TARGET = cube
TEMPLATE = app

SOURCES += main.cpp

contains(QT_CONFIG, opengl) {
    message(Building with OpenGL support.)
    QT += opengl

    SOURCES += mainwidget.cpp \
       geometryengine.cpp

    HEADERS += \
        mainwidget.h \
        geometryengine.h

    RESOURCES += \
        shaders.qrc \
        textures.qrc

    OTHER_FILES += \
        vshader.glsl \
        fshader.glsl
} else {
    message(OpenGL support is not available.)
}


simulator: warning(This example might not fully work on Simulator platform)
