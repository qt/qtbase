add_qt_test("tst_qmenu" RUN_SERIAL SOURCES tst_qmenu.cpp
    LIBRARIES
        Qt::GuiPrivate
        Qt::Widgets
        Qt::TestPrivate
)

extend_target("tst_qmenu" CONDITION APPLE_OSX
    SOURCES tst_qmenu_mac.mm
    LIBRARIES -lobjc
)

extend_target("tst_qmenu" CONDITION NOT APPLEL_OSX DEFINES QTEST_QPA_MOUSE_HANDLING)
