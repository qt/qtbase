# Generated from qshader.pro.

#####################################################################
## tst_qshader Test:
#####################################################################

qt_internal_add_test(tst_qshader
    SOURCES
        tst_qshader.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(qshader_resource_files
    "data"
)

qt_internal_add_resource(tst_qshader "qshader"
    PREFIX
        "/"
    FILES
        ${qshader_resource_files}
)

