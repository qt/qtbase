# Generated from qshader.pro.

#####################################################################
## tst_qshader Test:
#####################################################################

add_qt_test(tst_qshader
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

add_qt_resource(tst_qshader "qshader"
    PREFIX
        "/"
    FILES
        ${qshader_resource_files}
)

