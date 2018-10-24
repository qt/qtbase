add_qt_test("tst_qwidget_window" SOURCES tst_qwidget_window.cpp
    LIBRARIES
        Qt::Widgets
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::TestPrivate
)
