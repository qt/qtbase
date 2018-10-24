add_qt_test("tst_qaction" SOURCES tst_qaction.cpp
    LIBRARIES
        Qt::GuiPrivate
        Qt::CorePrivate
        Qt::Widgets
)
