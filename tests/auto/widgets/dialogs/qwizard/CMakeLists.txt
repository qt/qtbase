# Generated from qwizard.pro.

#####################################################################
## tst_qwizard Test:
#####################################################################

add_qt_test(tst_qwizard
    GUI
    SOURCES
        tst_qwizard.cpp
        tst_qwizard_2.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)

# Resources:
add_qt_resource(tst_qwizard "qwizard"
    PREFIX
        "/"
    FILES
        images/background.png
        images/banner.png
        images/logo.png
        images/watermark.png
)


#### Keys ignored in scope 1:.:.:qwizard.pro:<TRUE>:
# CONFIG = "testcase"
