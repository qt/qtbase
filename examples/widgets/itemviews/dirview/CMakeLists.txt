# Generated from dirview.pro.

#####################################################################
## dirview Binary:
#####################################################################

add_qt_executable(dirview
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/dirview"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/dirview"
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Widgets
)
