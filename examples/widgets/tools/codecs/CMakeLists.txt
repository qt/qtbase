# Generated from codecs.pro.

#####################################################################
## codecs Binary:
#####################################################################

add_qt_executable(codecs
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/codecs"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/codecs"
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h
        previewform.cpp previewform.h
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:codecs.pro:<NONE>:
# EXAMPLE_FILES = "encodedfiles"
