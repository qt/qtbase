# Generated from dialogs.pro.

if(QT_FEATURE_wizard)
    add_subdirectory(classwizard)
    add_subdirectory(trivialwizard)
endif()
add_subdirectory(extension)
add_subdirectory(findfiles)
add_subdirectory(standarddialogs)
add_subdirectory(tabdialog)
if(QT_FEATURE_wizard AND TARGET Qt::PrintSupport)
    add_subdirectory(licensewizard)
endif()
