add_qt_test("tst_qlabel" RUN_SERIAL SOURCES tst_qlabel.cpp
    LIBRARIES
        Qt::WidgetsPrivate
        Qt::CorePrivate
        Qt::GuiPrivate
)
