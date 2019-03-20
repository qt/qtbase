# Generated from embeddeddialogs.pro.

#####################################################################
## embeddeddialogs Binary:
#####################################################################

add_qt_executable(embeddeddialogs
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/embeddeddialogs"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/embeddeddialogs"
    SOURCES
        customproxy.cpp customproxy.h
        embeddeddialog.cpp embeddeddialog.h embeddeddialog.ui
        main.cpp
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(embeddeddialogs "embeddeddialogs" FILES
    No-Ones-Laughing-3.jpg)


## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:embeddeddialogs.pro:build_all AND NOT build_pass:
# CONFIG = "-build_all" "release"
