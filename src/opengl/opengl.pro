TARGET     = QtOpenGL
QT         = core-private gui-private
qtConfig(widgets): QT += widgets widgets-private

DEFINES   += QT_NO_USING_NAMESPACE QT_NO_FOREACH

QMAKE_DOCS = $$PWD/doc/qtopengl.qdocconf

qtConfig(opengl): CONFIG += opengl
qtConfig(opengles2): CONFIG += opengles2

HEADERS += \
    qopengldebug.h \
    qopenglwindow.h \
    qtopenglglobal.h

SOURCES += \
    qopenglwindow.cpp \
    qopengldebug.cpp

!qtConfig(opengles2) {
    HEADERS += \
        qopenglqueryhelper_p.h \
        qopengltimerquery.h

    SOURCES += qopengltimerquery.cpp
}

qtConfig(widgets) {
    HEADERS += qopenglwidget.h
    SOURCES += qopenglwidget.cpp
}

load(qt_module)
