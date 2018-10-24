add_qt_test("tst_qfiledialog"
    SOURCES tst_qfiledialog.cpp
    DEFINES SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/"
    LIBRARIES
        Qt::WidgetsPrivate
        Qt::CorePrivate
        Qt::GuiPrivate
)

