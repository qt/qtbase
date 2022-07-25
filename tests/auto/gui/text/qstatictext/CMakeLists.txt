# Generated from qstatictext.pro.

#####################################################################
## tst_qstatictext Test:
#####################################################################

qt_internal_add_test(tst_qstatictext
    SOURCES
        tst_qstatictext.cpp
    LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qstatictext CONDITION QT_FEATURE_private_tests
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
)
