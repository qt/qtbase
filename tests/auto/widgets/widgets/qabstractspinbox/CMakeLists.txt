add_qt_test("tst_qabstractspinbox" RUN_SERIAL SOURCES tst_qabstractspinbox.cpp
    LIBRARIES
        Qt::Widgets
        Qt::GuiPrivate
        Qt::CorePrivate
        Qt::TestPrivate
)
