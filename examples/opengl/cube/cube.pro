QT       += core gui widgets

TARGET = cube
TEMPLATE = app

SOURCES += main.cpp

contains(QT_CONFIG, opengl) {
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
}


simulator: warning(This example might not fully work on Simulator platform)
