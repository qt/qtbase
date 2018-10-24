add_qt_test("tst_qmessagebox" SOURCES tst_qmessagebox.cpp
    LIBRARIES
        Qt::GuiPrivate
        Qt::CorePrivate
        Qt::Widgets
)
