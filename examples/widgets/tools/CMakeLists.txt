# Generated from tools.pro.

add_subdirectory(codecs)
add_subdirectory(completer)
add_subdirectory(customcompleter)
if(QT_FEATURE_translation) # special case
    add_subdirectory(i18n)
endif()
add_subdirectory(regexp)
add_subdirectory(regularexpression)
add_subdirectory(settingseditor)
add_subdirectory(styleplugin)
add_subdirectory(treemodelcompleter)
add_subdirectory(undo)
add_subdirectory(undoframework)

if(QT_FEATURE_library) # special case
    add_subdirectory(echoplugin)
    # FIXME: Currently broken # special case
    # Fails to link (ld: error: undefined symbol: qt_static_plugin_BasicToolsPlugin()) # special case
    #add_subdirectory(plugandpaint) # special case
endif()
