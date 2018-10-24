add_qt_test("tst_qlistview"
    SOURCES
        tst_qlistview.cpp
    LIBRARIES
        Qt::GuiPrivate
        Qt::WidgetsPrivate
        Qt::CorePrivate
        Qt::TestPrivate
)

extend_target("tst_qlistview" CONDITION WIN32 AND NOT WINRT
    LIBRARIES -luser32
)
