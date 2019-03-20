# Generated from licensewizard.pro.

#####################################################################
## licensewizard Binary:
#####################################################################

add_qt_executable(licensewizard
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/dialogs/licensewizard"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/dialogs/licensewizard"
    SOURCES
        licensewizard.cpp licensewizard.h
        main.cpp
    LIBRARIES
        Qt::Widgets
        Qt::PrintSupport
)

# Resources:
add_qt_resource(licensewizard "licensewizard" FILES
    images/logo.png
    images/watermark.png)

