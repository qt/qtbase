# Generated from interview.pro.

#####################################################################
## interview Binary:
#####################################################################

add_qt_executable(interview
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/interview"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/interview"
    SOURCES
        main.cpp
        model.cpp model.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(interview "interview" PREFIX "/" FILES
    images/folder.png
    images/interview.png
    images/services.png)


#### Keys ignored in scope 1:.:interview.pro:<NONE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:interview.pro:build_all AND NOT build_pass:
# CONFIG = "-build_all" "release"
