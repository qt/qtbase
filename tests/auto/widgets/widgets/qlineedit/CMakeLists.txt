add_qt_test("tst_qlineedit" RUN_SERIAL SOURCES tst_qlineedit.cpp
    LIBRARIES
        Qt::GuiPrivate
        Qt::CorePrivate
        Qt::WidgetsPrivate
        Qt::TestPrivate
)

extend_target("tst_qlineedit" CONDITION APPLE_OSX LIBRARIES ${FWAppKit})
