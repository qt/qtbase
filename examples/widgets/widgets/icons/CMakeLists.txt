# Generated from icons.pro.

#####################################################################
## icons Binary:
#####################################################################

add_qt_executable(icons
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/icons"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/icons"
    SOURCES
        iconpreviewarea.cpp iconpreviewarea.h
        iconsizespinbox.cpp iconsizespinbox.h
        imagedelegate.cpp imagedelegate.h
        main.cpp
        mainwindow.cpp mainwindow.h
    DEFINES
        SRCDIR=\\\"/\\\"
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:icons.pro:<NONE>:
# EXAMPLE_FILES = "images/*"
