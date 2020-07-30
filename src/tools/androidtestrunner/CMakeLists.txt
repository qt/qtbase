# Generated from androidtestrunner.pro.

#####################################################################
## androidtestrunner App:
#####################################################################

qt_internal_add_app(androidtestrunner
    SOURCES
        main.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
        QT_NO_FOREACH
    PUBLIC_LIBRARIES
        Qt::Gui
)
set_target_properties(androidtestrunner PROPERTIES
    WIN32_EXECUTABLE FALSE
)

#### Keys ignored in scope 1:.:.:androidtestrunner.pro:<TRUE>:
# _OPTION = "host_build"

## Scopes:
#####################################################################

qt_extend_target(androidtestrunner CONDITION windows
    COMPILE_OPTIONS
        -U__STRICT_ANSI__
)
