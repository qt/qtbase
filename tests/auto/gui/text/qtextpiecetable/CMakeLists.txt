# Generated from qtextpiecetable.pro.

if(WIN32)
    return()
endif()
if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qtextpiecetable Test:
#####################################################################

qt_internal_add_test(tst_qtextpiecetable
    SOURCES
        ../qtextdocument/common.h
        tst_qtextpiecetable.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:.:qtextpiecetable.pro:<TRUE>:
# _REQUIREMENTS = "!win32" "qtConfig(private_tests)"
