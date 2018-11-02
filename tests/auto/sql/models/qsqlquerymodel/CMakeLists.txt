add_qt_test(tst_qsqlquerymodel
    SOURCES tst_qsqlquerymodel.cpp
    LIBRARIES
        Qt::Widgets
        Qt::CorePrivate
        Qt::SqlPrivate
)
