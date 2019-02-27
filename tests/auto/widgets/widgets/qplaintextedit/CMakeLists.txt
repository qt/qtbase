add_qt_test("tst_qplaintextedit" RUN_SERIAL SOURCES tst_qplaintextedit.cpp
    LIBRARIES
        Qt::WidgetsPrivate
        Qt::GuiPrivate
)

extend_target("tst_qplaintextedit" CONDITION APPLE_OSX LIBRARIES ${FWAppKit})
