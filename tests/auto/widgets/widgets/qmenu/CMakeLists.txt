# Generated from qmenu.pro.

#####################################################################
## tst_qmenu Test:
#####################################################################

qt_internal_add_test(tst_qmenu
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

qt_internal_extend_target(tst_qmenu CONDITION MACOS
    SOURCES
        tst_qmenu_mac.mm
    PUBLIC_LIBRARIES
        objc
)

qt_internal_extend_target(tst_qmenu CONDITION NOT MACOS
    DEFINES
        QTEST_QPA_MOUSE_HANDLING
)
