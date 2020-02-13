# Generated from qmdiarea.pro.

#####################################################################
## tst_qmdiarea Test:
#####################################################################

add_qt_test(tst_qmdiarea
    SOURCES
        tst_qmdiarea.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    INCLUDE_DIRECTORIES
        .
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)

## Scopes:
#####################################################################

extend_target(tst_qmdiarea CONDITION TARGET Qt::OpenGL
    PUBLIC_LIBRARIES
        Qt::OpenGL
)

extend_target(tst_qmdiarea CONDITION APPLE
    PUBLIC_LIBRARIES
        ${FWSecurity}
)
