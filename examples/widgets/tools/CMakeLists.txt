# Generated from tools.pro.

qt_internal_add_example(completer)
qt_internal_add_example(customcompleter)
if(QT_FEATURE_translation) # special case
    qt_internal_add_example(i18n)
endif()
qt_internal_add_example(regularexpression)
qt_internal_add_example(settingseditor)
qt_internal_add_example(styleplugin)
qt_internal_add_example(treemodelcompleter)
qt_internal_add_example(undo)
qt_internal_add_example(undoframework)

if(QT_FEATURE_library) # special case
    qt_internal_add_example(echoplugin)
    # special case begin
    if(QT_FEATURE_inputdialog)
        qt_internal_add_example(plugandpaint)
    endif()
    # special case end
endif()
