add_qt_test("tst_qstyle" SOURCES tst_qstyle.cpp LIBRARIES Qt::Widgets Qt::TestPrivate)

extend_target("tst_qstyle" CONDITION ANDROID AND NOT ANDROID_EMBEDDED SOURCES testdata.qrc)
