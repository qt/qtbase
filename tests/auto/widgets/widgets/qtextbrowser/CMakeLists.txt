add_qt_test("tst_qtextbrowser" RUN_SERIAL SOURCES tst_qtextbrowser.cpp LIBRARIES Qt::Widgets)

extend_target("tst_qtextbrowser" CONDITION builtin_testdata DEFINES BUILTIN_TESTDATA)
