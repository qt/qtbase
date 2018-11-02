add_qt_test(tst_qsqltablemodel
    SOURCES tst_qsqltablemodel.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::SqlPrivate
)
