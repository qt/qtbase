add_qt_test(tst_qsql
    SOURCES tst_qsql.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::SqlPrivate
)
