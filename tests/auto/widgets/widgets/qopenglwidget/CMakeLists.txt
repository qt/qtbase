add_qt_test("tst_qopenglwidget" RUN_SERIAL SOURCES tst_qopenglwidget.cpp
    LIBRARIES
        Qt::GuiPrivate
        Qt::CorePrivate
        Qt::Widgets
)
