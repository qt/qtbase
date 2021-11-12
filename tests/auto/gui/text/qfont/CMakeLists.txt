# Generated from qfont.pro.

#####################################################################
## tst_qfont Test:
#####################################################################

qt_internal_add_test(tst_qfont
    SOURCES
        tst_qfont.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
)

# Resources:
set(testfont_resource_files
    "datastream.515"
    "weirdfont.otf"
)

qt_internal_add_resource(tst_qfont "testfont"
    PREFIX
        "/"
    FILES
        ${testfont_resource_files}
)


## Scopes:
#####################################################################

qt_internal_extend_target(tst_qfont CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)
