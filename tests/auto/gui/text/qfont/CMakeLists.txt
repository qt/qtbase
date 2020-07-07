# Generated from qfont.pro.

#####################################################################
## tst_qfont Test:
#####################################################################

qt_add_test(tst_qfont
    SOURCES
        tst_qfont.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(testfont_resource_files
    "weirdfont.otf"
)

qt_add_resource(tst_qfont "testfont"
    PREFIX
        "/"
    FILES
        ${testfont_resource_files}
)


## Scopes:
#####################################################################

qt_extend_target(tst_qfont CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)
