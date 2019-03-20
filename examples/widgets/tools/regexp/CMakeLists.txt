# Generated from regexp.pro.

#####################################################################
## regexp Binary:
#####################################################################

add_qt_executable(regexp
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/regexp"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/regexp"
    SOURCES
        main.cpp
        regexpdialog.cpp regexpdialog.h
    LIBRARIES
        Qt::Widgets
)
