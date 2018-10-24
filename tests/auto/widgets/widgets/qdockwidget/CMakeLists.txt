add_qt_test("tst_qdockwidget" RUN_SERIAL SOURCES tst_qdockwidget.cpp
    LIBRARIES
        Qt::WidgetsPrivate
        Qt::CorePrivate
        Qt::GuiPrivate
)
