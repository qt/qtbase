# Generated from dropsite.pro.

#####################################################################
## dropsite Binary:
#####################################################################

add_qt_executable(dropsite
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/draganddrop/dropsite"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/draganddrop/dropsite"
    SOURCES
        droparea.cpp droparea.h
        dropsitewindow.cpp dropsitewindow.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)
