# Generated from qcombobox.pro.

#####################################################################
## tst_qcombobox Test:
#####################################################################

# Collect test data
list(APPEND test_data "qtlogo.png")
list(APPEND test_data "qtlogoinverted.png")

qt_internal_add_test(tst_qcombobox
    SOURCES
        tst_qcombobox.cpp
    DEFINES
        QTEST_QPA_MOUSE_HANDLING
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
    TESTDATA ${test_data}
)
