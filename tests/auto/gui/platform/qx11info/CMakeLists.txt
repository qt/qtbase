qt_internal_add_test(tst_qx11info
    SOURCES
        tst_qx11info.cpp
    LIBRARIES
        Qt::GuiPrivate
        XCB::XCB
)
