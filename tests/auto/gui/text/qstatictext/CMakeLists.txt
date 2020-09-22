# Generated from qstatictext.pro.

#####################################################################
## tst_qstatictext Test:
#####################################################################

qt_internal_add_test(tst_qstatictext
    SOURCES
        tst_qstatictext.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qstatictext CONDITION QT_FEATURE_private_tests
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
)
