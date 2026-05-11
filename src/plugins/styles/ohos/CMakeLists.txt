# Generated from ohos.pro.

#####################################################################
## QOhosStylePlugin Plugin:
#####################################################################

qt_internal_add_plugin(QOhosStylePlugin
    OUTPUT_NAME qohosstyle
    PLUGIN_TYPE styles
    SOURCES
        main.cpp
        qohosstyle.cpp qohosstyle_p.h
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::WidgetsPrivate
)

# Resources:
set(ohos_resource_files
    "resources/ic_gallery_material_select_checkbo.svg"
)

qt_internal_add_resource(QOhosStylePlugin "ohos"
    PREFIX
        "/"
    FILES
        ${ohos_resource_files}
)


#### Keys ignored in scope 1:.:.:ohos.pro:<TRUE>:
# DISTFILES = "ohosstyle.json"
