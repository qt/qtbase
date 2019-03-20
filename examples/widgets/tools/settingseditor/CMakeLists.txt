# Generated from settingseditor.pro.

#####################################################################
## settingseditor Binary:
#####################################################################

add_qt_executable(settingseditor
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/settingseditor"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/settingseditor"
    SOURCES
        locationdialog.cpp locationdialog.h
        main.cpp
        mainwindow.cpp mainwindow.h
        settingstree.cpp settingstree.h
        variantdelegate.cpp variantdelegate.h
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:settingseditor.pro:<NONE>:
# EXAMPLE_FILES = "inifiles"
