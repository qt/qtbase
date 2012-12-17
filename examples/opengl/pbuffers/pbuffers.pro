HEADERS += glwidget.h \
    cube.h
SOURCES += glwidget.cpp \
    main.cpp \
    cube.cpp
RESOURCES += pbuffers.qrc
QT += opengl widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/pbuffers
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)

contains(QT_CONFIG, opengles.) {
    contains(QT_CONFIG, angle): \
        warning("Qt was built with ANGLE, which provides only OpenGL ES 2.0 on top of DirectX 9.0c")
    error("This example requires Qt to be configured with -opengl desktop")
}
