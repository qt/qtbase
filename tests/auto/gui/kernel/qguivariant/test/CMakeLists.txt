# Generated from test.pro.

#####################################################################
## tst_qguivariant Test:
#####################################################################

qt_internal_add_test(tst_qguivariant
    SOURCES
        tst_qguivariant.cpp
    INCLUDE_DIRECTORIES
        ../../../../other/qvariant_common
    PUBLIC_LIBRARIES
        Qt::Gui
)

# Resources:
set(qguivariant_resource_files
    "data"
)

qt_internal_add_resource(tst_qguivariant "qguivariant"
    PREFIX
        "/"
    FILES
        ${qguivariant_resource_files}
)

