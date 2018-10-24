add_qt_test("tst_qlistwidget"
    SOURCES
        tst_qlistwidget.cpp
    LIBRARIES
        Qt::WidgetsPrivate
        Qt::CorePrivate
        Qt::GuiPrivate
)
