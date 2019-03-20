# Generated from fetchmore.pro.

#####################################################################
## fetchmore Binary:
#####################################################################

add_qt_executable(fetchmore
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/fetchmore"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/fetchmore"
    SOURCES
        filelistmodel.cpp filelistmodel.h
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
)
