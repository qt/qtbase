# Generated from qshader.pro.

#####################################################################
## tst_qshader Test:
#####################################################################

# Resources:
file(GLOB_RECURSE qshader_resource_files
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "data/*"
)

qt_internal_add_test(tst_qshader
    SOURCES
        tst_qshader.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${qshader_resource_files}
    BUILTIN_TESTDATA
)
