# Generated from nsphotolibrarysupport.pro.

#####################################################################
## QIosOptionalPlugin_NSPhotoLibrary Plugin:
#####################################################################

qt_internal_add_plugin(QIosOptionalPlugin_NSPhotoLibraryPlugin
    OUTPUT_NAME qiosnsphotolibrarysupport
    PLUGIN_TYPE platforms/darwin
    CLASS_NAME QIosOptionalPlugin_NSPhotoLibrary
    DEFAULT_IF FALSE
    SOURCES
        plugin.mm
        qiosfileengineassetslibrary.h qiosfileengineassetslibrary.mm
        qiosfileenginefactory.h
        qiosimagepickercontroller.h qiosimagepickercontroller.mm
    LIBRARIES
        ${FWAssetsLibrary}
        ${FWFoundation}
        ${FWUIKit}
        Qt::Core
        Qt::Gui
        Qt::GuiPrivate
)

# special case begin
set_target_properties(QIosOptionalPlugin_NSPhotoLibraryPlugin
    PROPERTIES
        DISABLE_PRECOMPILE_HEADERS ON
)
# special case end


#### Keys ignored in scope 1:.:.:nsphotolibrarysupport.pro:<TRUE>:
# OTHER_FILES = "plugin.json"
# PLUGIN_EXTENDS = "-"

## Scopes:
#####################################################################
