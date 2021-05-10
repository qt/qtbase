qt_internal_add_test(tst_qx11info
    SOURCES
        tst_qx11info.cpp
    PUBLIC_LIBRARIES
        Qt::GuiPrivate
        XCB::XCB
)
