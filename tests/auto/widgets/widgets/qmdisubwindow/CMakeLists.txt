add_qt_test("tst_qmdisubwindow" RUN_SERIAL SOURCES tst_qmdisubwindow.cpp
    DEFINES
        QT_NO_CAST_TO_ASCII
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::WidgetsPrivate
)
