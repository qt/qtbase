add_qt_test(tst_qsqlrelationaltablemodel
    SOURCES tst_qsqlrelationaltablemodel.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::SqlPrivate
)
