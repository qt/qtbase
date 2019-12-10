TARGET     = QtOpenGL
QT         = core-private gui-private

DEFINES   += QT_NO_USING_NAMESPACE QT_NO_FOREACH

QMAKE_DOCS = $$PWD/doc/qtopengl.qdocconf

qtConfig(opengl): CONFIG += opengl
qtConfig(opengles2): CONFIG += opengles2

HEADERS += \
    qopengldebug.h \
    qtopenglglobal.h

SOURCES += \
    qopengldebug.cpp

load(qt_module)
