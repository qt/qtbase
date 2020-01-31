# Generated from qcombobox.pro.

#####################################################################
## tst_qcombobox Test:
#####################################################################

# Collect test data
list(APPEND test_data "qtlogo.png")
list(APPEND test_data "qtlogoinverted.png")

add_qt_test(tst_qcombobox
    SOURCES
        tst_qcombobox.cpp
    DEFINES
        QTEST_QPA_MOUSE_HANDLING
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
    TESTDATA ${test_data}
)
