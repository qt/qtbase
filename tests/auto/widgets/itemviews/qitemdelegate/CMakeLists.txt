add_qt_test("tst_qitemdelegate" SOURCES tst_qitemdelegate.cpp LIBRARIES Qt::WidgetsPrivate)

extend_target("tst_qitemdelegate" CONDITION WIN32 AND NOT WINRT LIBRARIES -luser32)
