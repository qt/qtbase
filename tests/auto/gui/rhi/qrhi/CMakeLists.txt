# Generated from qrhi.pro.

#####################################################################
## tst_qrhi Test:
#####################################################################

qt_internal_add_test(tst_qrhi
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

qt_internal_add_resource(tst_qrhi "qrhi"
    PREFIX
        "/"
    FILES
        ${qrhi_resource_files}
)

