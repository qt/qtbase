add_qt_test("tst_qgl" SOURCES qgl.qrc tst_qgl.cpp
    LIBRARIES
        Qt::WidgetsPrivate
        Qt::OpenGLPrivate
        Qt::GuiPrivate
        Qt::CorePrivate
)
