add_qt_test("tst_qglthreads" SOURCES tst_qglthreads.cpp tst_qglthreads.h
    LIBRARIES
        Qt::OpenGL
        Qt::Widgets
        Qt::GuiPrivate
        Qt::CorePrivate
)
