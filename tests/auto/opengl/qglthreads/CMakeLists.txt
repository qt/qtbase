# Generated from qglthreads.pro.

if(NOT TARGET Qt::OpenGL)
    return()
endif()

#####################################################################
## tst_qglthreads Test:
#####################################################################

add_qt_test(tst_qglthreads
    SOURCES
        tst_qglthreads.cpp tst_qglthreads.h
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::OpenGL
        Qt::Widgets
)

#### Keys ignored in scope 1:.:.:qglthreads.pro:<TRUE>:
# _REQUIREMENTS = "qtHaveModule(opengl)"
