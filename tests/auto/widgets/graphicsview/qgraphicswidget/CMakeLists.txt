add_qt_test("tst_qgraphicswidget" SOURCES tst_qgraphicswidget.cpp
    LIBRARIES
        Qt::WidgetsPrivate
        Qt::CorePrivate
        Qt::GuiPrivate
)
