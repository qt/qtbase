# Generated from test.pro.

#####################################################################
## tst_qguivariant Test:
#####################################################################

qt_add_test(tst_qguivariant
    SOURCES
        tst_qguivariant.cpp
    INCLUDE_DIRECTORIES
        ../../../../other/qvariant_common
    PUBLIC_LIBRARIES
        Qt::Gui
)

# Resources:
set(tst_qguivariant_resource_files
    "black.png"
    "black2.png"
)

qt_add_resource(tst_qguivariant "tst_qguivariant"
    PREFIX
        "/"
    FILES
        ${tst_qguivariant_resource_files}
)
set(qguivariant_resource_files
    "data"
)

qt_add_resource(tst_qguivariant "qguivariant"
    PREFIX
        "/"
    FILES
        ${qguivariant_resource_files}
)

