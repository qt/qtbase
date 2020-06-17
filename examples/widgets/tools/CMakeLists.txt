# Generated from tools.pro.

add_subdirectory(completer)
add_subdirectory(customcompleter)
if(QT_FEATURE_translation) # special case
    add_subdirectory(i18n)
endif()
add_subdirectory(regularexpression)
add_subdirectory(settingseditor)
add_subdirectory(styleplugin)
add_subdirectory(treemodelcompleter)
add_subdirectory(undo)
add_subdirectory(undoframework)

if(QT_FEATURE_library) # special case
    add_subdirectory(echoplugin)
    add_subdirectory(plugandpaint) # special case
endif()
