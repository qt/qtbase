# Generated from masterdetail.pro.

#####################################################################
## masterdetail Binary:
#####################################################################

add_qt_executable(masterdetail
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/sql/masterdetail"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/sql/masterdetail"
    SOURCES
        database.h
        dialog.cpp dialog.h
        main.cpp
        mainwindow.cpp mainwindow.h
    LIBRARIES
        Qt::Sql
        Qt::Widgets
        Qt::Xml
        Qt::Widgets
)

# Resources:
add_qt_resource(masterdetail "masterdetail" FILES
    images/icon.png
    images/image.png)


#### Keys ignored in scope 1:.:masterdetail.pro:<NONE>:
# EXAMPLE_FILES = "albumdetails.xml"
