# Qt gui library, opengl module

qtConfig(opengl): CONFIG += opengl
qtConfig(opengles2): CONFIG += opengles2

qtConfig(opengl) {
    HEADERS += opengl/qopengl.h \
               opengl/qopengl_p.h \
               opengl/qopenglfunctions.h \
               opengl/qopenglextensions_p.h \
               opengl/qopenglextrafunctions.h \
               opengl/qopenglprogrambinarycache_p.h

    SOURCES += opengl/qopengl.cpp \
               opengl/qopenglfunctions.cpp \
               opengl/qopenglprogrambinarycache.cpp
}

include(platform/platform.pri)
