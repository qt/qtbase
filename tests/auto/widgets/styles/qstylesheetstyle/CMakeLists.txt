# Generated from qstylesheetstyle.pro.

#####################################################################
## tst_qstylesheetstyle Test:
#####################################################################

# Resources:
set(resources_resource_files
    "images/testimage.png"
    "images/testimage@2x.png"
)

qt_internal_add_test(tst_qstylesheetstyle
    SOURCES
        tst_qstylesheetstyle.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
    TESTDATA ${resources_resource_files}
    BUILTIN_TESTDATA
)

#### Keys ignored in scope 1:.:.:qstylesheetstyle.pro:<TRUE>:
# _REQUIREMENTS = "qtConfig(private_tests)"
