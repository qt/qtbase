add_qt_test("tst_qlayout" SOURCES tst_qlayout.cpp LIBRARIES Qt::WidgetsPrivate Qt::TestPrivate)

extend_target("tst_qlayout" CONDITION ANDROID AND NOT ANDROID_EMBEDDED SOURCES testdata.qrc)
