QT += opengl widgets

VPATH += ../shared
INCLUDEPATH += ../shared

HEADERS = bubble.h \
    glwidget.h \
    qtlogo.h

SOURCES = bubble.cpp \
    glwidget.cpp \
    main.cpp \
    qtlogo.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/overpainting
INSTALLS += target

contains(QT_CONFIG, opengles.) {
    contains(QT_CONFIG, angle): \
        warning("Qt was built with ANGLE, which provides only OpenGL ES 2.0 on top of DirectX 9.0c")
    error("This example requires Qt to be configured with -opengl desktop")
}
