add_qt_test("tst_qgraphicsview"
    SOURCES
        tst_qgraphicsview.cpp tst_qgraphicsview.h
        tst_qgraphicsview_2.cpp
    DEFINES
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::WidgetsPrivate
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::TestPrivate
)
