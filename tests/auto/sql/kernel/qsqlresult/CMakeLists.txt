add_qt_test(tst_qsqlresult
    SOURCES
        testsqldriver.h
        tst_qsqlresult.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::SqlPrivate
)
