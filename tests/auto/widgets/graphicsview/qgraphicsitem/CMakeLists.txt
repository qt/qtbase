add_qt_test("tst_qgraphicsitem" SOURCES tst_qgraphicsitem.cpp
    DEFINES
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::WidgetsPrivate
        Qt::TestPrivate
        Qt::CorePrivate
        Qt::GuiPrivate
)

extend_target("tst_qgraphicsitem" CONDITION WIN32 AND NOT WINRT LIBRARIES -luser32)
