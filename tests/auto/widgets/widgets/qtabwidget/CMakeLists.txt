add_qt_test("tst_qtabwidget" RUN_SERIAL SOURCES tst_qtabwidget.cpp LIBRARIES Qt::WidgetsPrivate)

extend_target("tst_qtabwidget" CONDITION WIN32 AND NOT WINRT LIBRARIES -luser32)
