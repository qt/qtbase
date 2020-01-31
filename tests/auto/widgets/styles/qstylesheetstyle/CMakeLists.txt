# Generated from qstylesheetstyle.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qstylesheetstyle Test:
#####################################################################

add_qt_test(tst_qstylesheetstyle
    SOURCES
        tst_qstylesheetstyle.cpp
    LIBRARIES
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)

# Resources:
set(resources_resource_files
    "images/testimage.png"
    "images/testimage@2x.png"
)

add_qt_resource(tst_qstylesheetstyle "resources"
    PREFIX
        "/"
    FILES
        ${resources_resource_files}
)


#### Keys ignored in scope 1:.:.:qstylesheetstyle.pro:<TRUE>:
# _REQUIREMENTS = "qtConfig(private_tests)"
