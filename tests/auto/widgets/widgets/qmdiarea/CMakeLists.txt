add_qt_test("tst_qmdiarea" RUN_SERIAL SOURCES tst_qmdiarea.cpp
    DEFINES
        QT_NO_CAST_TO_ASCII
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::GuiPrivate
        Qt::Widgets
)

extend_target("tst_qmdiarea" CONDITION TARGET Qt::OpenGL LIBRARIES Qt::OpenGL)
extend_target("tst_qmdiarea" CONDITION APPLE_OSX LIBRARIES ${FWSecurity})
