# Generated from qglfunctions.pro.

if(NOT TARGET Qt::OpenGL)
    return()
endif()

#####################################################################
## tst_qglfunctions Test:
#####################################################################

add_qt_test(tst_qglfunctions
    SOURCES
        tst_qglfunctions.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::OpenGL
        Qt::Widgets
)

#### Keys ignored in scope 1:.:.:qglfunctions.pro:<TRUE>:
# _REQUIREMENTS = "qtHaveModule(opengl)"
