QT += opengl widgets

contains(QT_CONFIG, opengles.|angle|dynamicgl):error("This example requires Qt to be configured with -opengl desktop")

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

wince {
    DEPLOYMENT_PLUGIN += qjpeg
}
