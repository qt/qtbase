# Generated from nsphotolibrarysupport.pro.

#####################################################################
## qiosnsphotolibrarysupport Plugin:
#####################################################################

add_qt_plugin(qiosnsphotolibrarysupport
    TYPE platforms/darwin
    CLASS_NAME QIosOptionalPlugin_NSPhotoLibrary
    SOURCES
        plugin.mm
        qiosfileengineassetslibrary.h qiosfileengineassetslibrary.mm
        qiosfileenginefactory.h
        qiosimagepickercontroller.h qiosimagepickercontroller.mm
    LIBRARIES
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        ${FWAssetsLibrary}
        ${FWFoundation}
        ${FWUIKit}
        Qt::Core
        Qt::Gui
)

#### Keys ignored in scope 1:.:.:nsphotolibrarysupport.pro:<TRUE>:
# OTHER_FILES = "plugin.json"
# PLUGIN_EXTENDS = "-"

## Scopes:
#####################################################################
