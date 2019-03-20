# Generated from completer.pro.

#####################################################################
## completer Binary:
#####################################################################

add_qt_executable(completer
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/completer"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/completer"
    SOURCES
        fsmodel.cpp fsmodel.h
        main.cpp
        mainwindow.cpp mainwindow.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(completer "completer" PREFIX "/" FILES
    resources/countries.txt
    resources/wordlist.txt)

