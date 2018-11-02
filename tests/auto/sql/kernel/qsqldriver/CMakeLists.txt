add_qt_test(tst_qsqldriver
    SOURCES tst_qsqldriver.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::SqlPrivate
)
