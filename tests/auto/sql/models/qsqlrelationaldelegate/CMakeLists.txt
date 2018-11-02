add_qt_test(tst_qsqlrelationaldelegate
    SOURCES tst_qsqlrelationaldelegate.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::SqlPrivate
        Qt::Widgets
)
