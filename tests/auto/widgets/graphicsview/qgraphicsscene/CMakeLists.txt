add_qt_test("tst_qgraphicsscene"
    SOURCES
        images.qrc
        testdata.qrc
        tst_qgraphicsscene.cpp
    DEFINES
        SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::WidgetsPrivate
        Qt::CorePrivate
        Qt::GuiPrivate
)

extend_target("tst_qgraphicsscene" CONDITION WIN32 AND NOT WINRT LIBRARIES -luser32)
