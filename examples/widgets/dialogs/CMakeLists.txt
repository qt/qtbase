# Generated from dialogs.pro.

add_subdirectory(extension)
add_subdirectory(findfiles)
if(TARGET Qt::PrintSupport AND QT_FEATURE_wizard) # special case
    add_subdirectory(licensewizard)
endif()
add_subdirectory(standarddialogs)
add_subdirectory(tabdialog)

if(QT_FEATURE_wizard) # special case
    add_subdirectory(classwizard)
    add_subdirectory(trivialwizard)
endif()
