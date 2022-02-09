# Generated from qwizard.pro.

#####################################################################
## tst_qwizard Test:
#####################################################################

# Resources:
set(qwizard_resource_files
    "images/background.png"
    "images/banner.png"
    "images/logo.png"
    "images/watermark.png"
)

qt_internal_add_test(tst_qwizard
    SOURCES
        tst_qwizard.cpp
        tst_qwizard_2.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
    TESTDATA ${qwizard_resource_files}
    BUILTIN_TESTDATA
)

