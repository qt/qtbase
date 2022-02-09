# Generated from qrhi.pro.

#####################################################################
## tst_qrhi Test:
#####################################################################

# Resources:
file(GLOB_RECURSE qrhi_resource_files
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    data/*
)

qt_internal_add_test(tst_qrhi
    SOURCES
        tst_qrhi.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${qrhi_resource_files}
    BUILTIN_TESTDATA
)
