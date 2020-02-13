# Generated from qmenu.pro.

#####################################################################
## tst_qmenu Test:
#####################################################################

add_qt_test(tst_qmenu
    SOURCES
        tst_qmenu.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::Widgets
)

## Scopes:
#####################################################################

extend_target(tst_qmenu CONDITION APPLE_OSX
    SOURCES
        tst_qmenu_mac.mm
    PUBLIC_LIBRARIES
        objc
)

extend_target(tst_qmenu CONDITION NOT APPLE_OSX
    DEFINES
        QTEST_QPA_MOUSE_HANDLING
)
