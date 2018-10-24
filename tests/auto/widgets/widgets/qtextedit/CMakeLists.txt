add_qt_test("tst_qtextedit" RUN_SERIAL SOURCES tst_qtextedit.cpp
    LIBRARIES
        Qt::WidgetsPrivate
        Qt::GuiPrivate
        Qt::CorePrivate
)

extend_target("tst_qtextedit" CONDITION APPLE_OSX LIBRARIES FWAppKit)
