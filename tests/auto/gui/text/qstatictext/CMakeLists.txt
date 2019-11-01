# Generated from qstatictext.pro.

#####################################################################
## tst_qstatictext Test:
#####################################################################

add_qt_test(tst_qstatictext
    SOURCES
        tst_qstatictext.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

extend_target(tst_qstatictext CONDITION QT_FEATURE_private_tests
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
)
