#-------------------------------------------------
#
# Project created by QtCreator 2010-06-23T12:55:35
#
#-------------------------------------------------

QT       += core gui

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

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
maemo5: include($$QT_SOURCE_TREE/examples/maemo5pkgrules.pri)

symbian: warning(This example might not fully work on Symbian platform)
simulator: warning(This example might not fully work on Simulator platform)
