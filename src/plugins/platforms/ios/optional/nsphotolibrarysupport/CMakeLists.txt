# Generated from nsphotolibrarysupport.pro.

#####################################################################
## QIosOptionalPlugin_NSPhotoLibrary Plugin:
#####################################################################

qt_internal_add_plugin(QIosOptionalPlugin_NSPhotoLibrary
    OUTPUT_NAME qiosnsphotolibrarysupport
    TYPE platforms/darwin
    DEFAULT_IF FALSE
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

# special case begin
set_target_properties(QIosOptionalPlugin_NSPhotoLibrary
    PROPERTIES
        DISABLE_PRECOMPILE_HEADERS ON
)
# special case end


#### Keys ignored in scope 1:.:.:nsphotolibrarysupport.pro:<TRUE>:
# OTHER_FILES = "plugin.json"
# PLUGIN_EXTENDS = "-"

## Scopes:
#####################################################################
