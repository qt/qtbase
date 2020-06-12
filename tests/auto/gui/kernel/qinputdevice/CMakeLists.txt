#####################################################################
## tst_qinputdevice Test:
#####################################################################

add_qt_test(tst_qinputdevice
    SOURCES
        tst_qinputdevice.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)
