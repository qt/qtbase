# Generated from frozencolumn.pro.

#####################################################################
## frozencolumn Binary:
#####################################################################

add_qt_executable(frozencolumn
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/frozencolumn"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/frozencolumn"
    SOURCES
        freezetablewidget.cpp freezetablewidget.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(frozencolumn "grades" PREFIX "/" FILES
    grades.txt)

