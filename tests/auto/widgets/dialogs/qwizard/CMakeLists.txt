# Generated from qwizard.pro.

#####################################################################
## tst_qwizard Test:
#####################################################################

add_qt_test(tst_qwizard
    SOURCES
        tst_qwizard.cpp
        tst_qwizard_2.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)

# Resources:
set(qwizard_resource_files
    "images/background.png"
    "images/banner.png"
    "images/logo.png"
    "images/watermark.png"
)

add_qt_resource(tst_qwizard "qwizard"
    PREFIX
        "/"
    FILES
        ${qwizard_resource_files}
)

