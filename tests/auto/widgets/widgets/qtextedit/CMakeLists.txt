# Generated from qtextedit.pro.

#####################################################################
## tst_qtextedit Test:
#####################################################################

# Collect test data
list(APPEND test_data "fullWidthSelection")

add_qt_test(tst_qtextedit
    SOURCES
        tst_qtextedit.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

extend_target(tst_qtextedit CONDITION APPLE_OSX
    PUBLIC_LIBRARIES
        ${FWAppKit}
)
