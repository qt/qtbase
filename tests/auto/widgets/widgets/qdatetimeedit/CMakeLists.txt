add_qt_test("tst_qdatetimeedit" RUN_SERIAL SOURCES tst_qdatetimeedit.cpp
    LIBRARIES
        Qt::Widgets
        Qt::CorePrivate
        Qt::WidgetsPrivate
)
