# Generated from qmenubar.pro.

#####################################################################
## tst_qmenubar Test:
#####################################################################

add_qt_test(tst_qmenubar
    SOURCES
        tst_qmenubar.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::TestPrivate
        Qt::Widgets
)

## Scopes:
#####################################################################

extend_target(tst_qmenubar CONDITION APPLE_OSX
    SOURCES
        tst_qmenubar_mac.mm
    PUBLIC_LIBRARIES
        ${FWAppKit}
)
