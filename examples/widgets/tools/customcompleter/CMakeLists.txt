# Generated from customcompleter.pro.

#####################################################################
## customcompleter Binary:
#####################################################################

add_qt_executable(customcompleter
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/customcompleter"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/customcompleter"
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h
        textedit.cpp textedit.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(customcompleter "customcompleter" PREFIX "/" FILES
    resources/wordlist.txt)

