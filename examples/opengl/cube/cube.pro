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

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/cube
sources.files = $$SOURCES $$HEADERS $$RESOURCES cube.png *.glsl *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/opengl/cube
INSTALLS += target sources

simulator: warning(This example might not fully work on Simulator platform)
