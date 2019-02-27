add_qt_test("tst_qmenubar" RUN_SERIAL SOURCES tst_qmenubar.cpp LIBRARIES Qt::Widgets Qt::TestPrivate)

extend_target("tst_qmenubar" CONDITION APPLE_OSX
    SOURCES tst_qmenubar_mac.mm
    LIBRARIES ${FWAppKit}
)
