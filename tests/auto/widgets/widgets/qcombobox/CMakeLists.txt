add_qt_test("tst_qcombobox" RUN_SERIAL SOURCES tst_qcombobox.cpp
    DEFINES QTEST_QPA_MOUSE_HANDLING
    LIBRARIES
        Qt::WidgetsPrivate
        Qt::GuiPrivate
        Qt::CorePrivate
        Qt::TestPrivate
)
