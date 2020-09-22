# Generated from qiconhighdpi.pro.

#####################################################################
## tst_qiconhighdpi Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    icons/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qiconhighdpi
    SOURCES
        tst_qiconhighdpi.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
    TESTDATA ${test_data}
)

# Resources:
set(tst_qiconhighdpi_resource_files
    "icons/misc/button.9.png"
    "icons/misc/button@2x.9.png"
    "icons/testtheme/16x16/actions/appointment-new.png"
    "icons/testtheme/22x22/actions/appointment-new.png"
    "icons/testtheme/22x22@2/actions/appointment-new.png"
    "icons/testtheme/index.theme"
)

qt_internal_add_resource(tst_qiconhighdpi "tst_qiconhighdpi"
    PREFIX
        "/"
    FILES
        ${tst_qiconhighdpi_resource_files}
)

