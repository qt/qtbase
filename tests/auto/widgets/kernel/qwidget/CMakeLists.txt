add_qt_test("tst_qwidget"
    SOURCES
        qwidget.qrc
        tst_qwidget.cpp
    LIBRARIES
        Qt::Widgets
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::WidgetsPrivate
        Qt::TestPrivate
)

extend_target("tst_qwidget" CONDITION APPLE_OSX
    SOURCES tst_qwidget_mac_helpers.mm
    LIBRARIES ${FWSecurity} ${FWAppKit}
)
