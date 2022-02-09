# Generated from qfont.pro.

#####################################################################
## tst_qfont Test:
#####################################################################

# Resources:
set(testfont_resource_files
    "datastream.515"
    "weirdfont.otf"
)

qt_internal_add_test(tst_qfont
    SOURCES
        tst_qfont.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
    TESTDATA ${testfont_resource_files}
    BUILTIN_TESTDATA
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qfont CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)
