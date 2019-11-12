# Generated from qglbuffer.pro.

if(NOT TARGET Qt::OpenGL)
    return()
endif()

#####################################################################
## tst_qglbuffer Test:
#####################################################################

add_qt_test(tst_qglbuffer
    SOURCES
        tst_qglbuffer.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::OpenGL
        Qt::Widgets
)

#### Keys ignored in scope 1:.:.:qglbuffer.pro:<TRUE>:
# _REQUIREMENTS = "qtHaveModule(opengl)"
