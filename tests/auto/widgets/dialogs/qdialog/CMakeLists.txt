add_qt_test("tst_qdialog"
    SOURCES
        tst_qdialog.cpp
    LIBRARIES
        Qt::Widgets
        Qt::GuiPrivate
        Qt::CorePrivate
)
