# Generated from qrhi.pro.

#####################################################################
## tst_qrhi Test:
#####################################################################

add_qt_test(tst_qrhi
    SOURCES
        tst_qrhi.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(qrhi_resource_files
    "data"
)

add_qt_resource(tst_qrhi "qrhi"
    PREFIX
        "/"
    FILES
        ${qrhi_resource_files}
)

