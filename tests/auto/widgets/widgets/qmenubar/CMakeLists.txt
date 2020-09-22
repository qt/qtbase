# Generated from qmenubar.pro.

#####################################################################
## tst_qmenubar Test:
#####################################################################

qt_internal_add_test(tst_qmenubar
    SOURCES
        tst_qmenubar.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::TestPrivate
        Qt::Widgets
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qmenubar CONDITION MACOS
    SOURCES
        tst_qmenubar_mac.mm
    PUBLIC_LIBRARIES
        ${FWAppKit}
)
