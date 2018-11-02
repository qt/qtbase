add_qt_test(tst_qsqldatabase
    SOURCES tst_qsqldatabase.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::SqlPrivate
)
