# Generated from printsupport.pro.

if(NOT QT_FEATURE_printer OR NOT QT_FEATURE_printdialog)
    return()
endif()
add_subdirectory(dialogs)
add_subdirectory(kernel)
