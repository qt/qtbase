# Generated from nsphotolibrarysupport.pro.

#####################################################################
## QIosOptionalPlugin_NSPhotoLibrary Plugin:
#####################################################################

qt_internal_add_plugin(QIosOptionalPlugin_NSPhotoLibrary
    OUTPUT_NAME qiosnsphotolibrarysupport
    TYPE platforms/darwin
    SOURCES
        plugin.mm
        qiosfileengineassetslibrary.h qiosfileengineassetslibrary.mm
        qiosfileenginefactory.h
        qiosimagepickercontroller.h qiosimagepickercontroller.mm
    PUBLIC_LIBRARIES
        ${FWAssetsLibrary}
        ${FWFoundation}
        ${FWUIKit}
        Qt::Core
        Qt::Gui
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:.:nsphotolibrarysupport.pro:<TRUE>:
# OTHER_FILES = "plugin.json"
# PLUGIN_EXTENDS = "-"

## Scopes:
#####################################################################
