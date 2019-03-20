# Generated from fingerpaint.pro.

#####################################################################
## fingerpaint Binary:
#####################################################################

add_qt_executable(fingerpaint
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/touch/fingerpaint"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/touch/fingerpaint"
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h
        scribblearea.cpp scribblearea.h
    LIBRARIES
        Qt::Widgets
)

## Scopes:
#####################################################################

extend_target(fingerpaint CONDITION TARGET Qt::PrintSupport
    LIBRARIES
        Qt::PrintSupport
)
