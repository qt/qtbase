QT += opengl widgets

contains(QT_CONFIG, opengles.) {
    contains(QT_CONFIG, angle): \
        warning("Qt was built with ANGLE, which provides only OpenGL ES 2.0 on top of DirectX 9.0c")
    error("This example requires Qt to be configured with -opengl desktop")
}

HEADERS += 3rdparty/fbm.h \
           glbuffers.h \
           glextensions.h \
           gltrianglemesh.h \
           qtbox.h \
           roundedbox.h \
           scene.h \
           trackball.h
SOURCES += 3rdparty/fbm.c \
           glbuffers.cpp \
           glextensions.cpp \
           main.cpp \
           qtbox.cpp \
           roundedbox.cpp \
           scene.cpp \
           trackball.cpp

RESOURCES += boxes.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/graphicsview/boxes
INSTALLS += target

wince*: {
    DEPLOYMENT_PLUGIN += qjpeg
}

win32-msvc* {
    QMAKE_CXXFLAGS -= -Zm200
    QMAKE_CFLAGS -= -Zm200
    QMAKE_CXXFLAGS += -Zm500
    QMAKE_CFLAGS += -Zm500
}
