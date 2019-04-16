# Generated from codecs.pro.

#####################################################################
## codecs Binary:
#####################################################################

add_qt_executable(codecs
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/codecs"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/codecs"
    SOURCES
        encodingdialog.cpp encodingdialog.h
        main.cpp
        mainwindow.cpp mainwindow.h
        previewform.cpp previewform.h
    PUBLIC_LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(codecs "codecs" PREFIX "/" FILES
    images/editcopy.png)


#### Keys ignored in scope 1:.:.:codecs.pro:<TRUE>:
# EXAMPLE_FILES = "encodedfiles"
