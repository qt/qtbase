# Qt gui library, opengl module

qtConfig(opengl): CONFIG += opengl
qtConfig(opengles2): CONFIG += opengles2

qtConfig(opengl) {

    HEADERS += opengl/qopengl.h \
               opengl/qopengl_p.h \
               opengl/qopenglfunctions.h \
               opengl/qopenglextensions_p.h \
               opengl/qopenglversionfunctions.h \
               opengl/qopenglversionfunctionsfactory_p.h \
               opengl/qopenglversionprofile.h \
               opengl/qopenglextrafunctions.h \
               opengl/qopenglprogrambinarycache_p.h

    SOURCES += opengl/qopengl.cpp \
               opengl/qopenglfunctions.cpp \
               opengl/qopenglversionfunctions.cpp \
               opengl/qopenglversionfunctionsfactory.cpp \
               opengl/qopenglversionprofile.cpp \
               opengl/qopenglprogrambinarycache.cpp

    !qtConfig(opengles2) {
        HEADERS += opengl/qopenglfunctions_1_0.h \
                   opengl/qopenglfunctions_1_1.h \
                   opengl/qopenglfunctions_1_2.h \
                   opengl/qopenglfunctions_1_3.h \
                   opengl/qopenglfunctions_1_4.h \
                   opengl/qopenglfunctions_1_5.h \
                   opengl/qopenglfunctions_2_0.h \
                   opengl/qopenglfunctions_2_1.h \
                   opengl/qopenglfunctions_3_0.h \
                   opengl/qopenglfunctions_3_1.h \
                   opengl/qopenglfunctions_3_2_core.h \
                   opengl/qopenglfunctions_3_3_core.h \
                   opengl/qopenglfunctions_4_0_core.h \
                   opengl/qopenglfunctions_4_1_core.h \
                   opengl/qopenglfunctions_4_2_core.h \
                   opengl/qopenglfunctions_4_3_core.h \
                   opengl/qopenglfunctions_4_4_core.h \
                   opengl/qopenglfunctions_4_5_core.h \
                   opengl/qopenglfunctions_3_2_compatibility.h \
                   opengl/qopenglfunctions_3_3_compatibility.h \
                   opengl/qopenglfunctions_4_0_compatibility.h \
                   opengl/qopenglfunctions_4_1_compatibility.h \
                   opengl/qopenglfunctions_4_2_compatibility.h \
                   opengl/qopenglfunctions_4_3_compatibility.h \
                   opengl/qopenglfunctions_4_4_compatibility.h \
                   opengl/qopenglfunctions_4_5_compatibility.h

        SOURCES += opengl/qopenglfunctions_1_0.cpp \
                   opengl/qopenglfunctions_1_1.cpp \
                   opengl/qopenglfunctions_1_2.cpp \
                   opengl/qopenglfunctions_1_3.cpp \
                   opengl/qopenglfunctions_1_4.cpp \
                   opengl/qopenglfunctions_1_5.cpp \
                   opengl/qopenglfunctions_2_0.cpp \
                   opengl/qopenglfunctions_2_1.cpp \
                   opengl/qopenglfunctions_3_0.cpp \
                   opengl/qopenglfunctions_3_1.cpp \
                   opengl/qopenglfunctions_3_2_core.cpp \
                   opengl/qopenglfunctions_3_3_core.cpp \
                   opengl/qopenglfunctions_4_0_core.cpp \
                   opengl/qopenglfunctions_4_1_core.cpp \
                   opengl/qopenglfunctions_4_2_core.cpp \
                   opengl/qopenglfunctions_4_3_core.cpp \
                   opengl/qopenglfunctions_4_4_core.cpp \
                   opengl/qopenglfunctions_4_5_core.cpp \
                   opengl/qopenglfunctions_3_2_compatibility.cpp \
                   opengl/qopenglfunctions_3_3_compatibility.cpp \
                   opengl/qopenglfunctions_4_0_compatibility.cpp \
                   opengl/qopenglfunctions_4_1_compatibility.cpp \
                   opengl/qopenglfunctions_4_2_compatibility.cpp \
                   opengl/qopenglfunctions_4_3_compatibility.cpp \
                   opengl/qopenglfunctions_4_4_compatibility.cpp \
                   opengl/qopenglfunctions_4_5_compatibility.cpp
    }

    qtConfig(opengles2) {
        HEADERS += opengl/qopenglfunctions_es2.h

        SOURCES += opengl/qopenglfunctions_es2.cpp
    }
}
