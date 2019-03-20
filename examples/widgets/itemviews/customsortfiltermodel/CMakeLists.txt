# Generated from customsortfiltermodel.pro.

#####################################################################
## customsortfiltermodel Binary:
#####################################################################

add_qt_executable(customsortfiltermodel
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/customsortfiltermodel"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/customsortfiltermodel"
    SOURCES
        filterwidget.cpp filterwidget.h
        main.cpp
        mysortfilterproxymodel.cpp mysortfilterproxymodel.h
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(customsortfiltermodel "customsortfiltermodel" PREFIX "/" FILES
    images/find.png)

