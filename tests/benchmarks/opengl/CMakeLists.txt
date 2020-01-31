# Generated from opengl.pro.

#####################################################################
## tst_bench_opengl Binary:
#####################################################################

add_qt_benchmark(tst_bench_opengl
    SOURCES
        main.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::OpenGLPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::OpenGL
        Qt::Test
)

#### Keys ignored in scope 1:.:.:opengl.pro:<TRUE>:
# TEMPLATE = "app"
