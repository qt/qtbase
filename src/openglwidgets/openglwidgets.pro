TARGET = QtOpenGLWidgets
QT = opengl widgets
QT_PRIVATE += opengl-private widgets-private

DEFINES += QT_NO_USING_NAMESPACE QT_NO_FOREACH

HEADERS += \
    qopenglwidget.h

SOURCES += \
    qopenglwidget.cpp

load(qt_module)
