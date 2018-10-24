add_qt_test("tst_qtableview" SOURCES tst_qtableview.cpp
    LIBRARIES
        Qt::WidgetsPrivate
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::TestPrivate
)
