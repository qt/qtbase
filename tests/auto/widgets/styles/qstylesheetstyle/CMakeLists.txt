# Generated from qstylesheetstyle.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qstylesheetstyle Test:
#####################################################################

qt_internal_add_test(tst_qstylesheetstyle
    SOURCES
        tst_qstylesheetstyle.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)

# Resources:
set(resources_resource_files
    "images/testimage.png"
    "images/testimage@2x.png"
)

qt_internal_add_resource(tst_qstylesheetstyle "resources"
    PREFIX
        "/"
    FILES
        ${resources_resource_files}
)


#### Keys ignored in scope 1:.:.:qstylesheetstyle.pro:<TRUE>:
# _REQUIREMENTS = "qtConfig(private_tests)"
