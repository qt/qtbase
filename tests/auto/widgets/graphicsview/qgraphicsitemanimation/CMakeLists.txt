add_qt_test("tst_qgraphicsitemanimation" SOURCES tst_qgraphicsitemanimation.cpp
    DEFINES
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::Widgets
)
