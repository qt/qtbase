# Generated from androiddeployqt.pro.

#####################################################################
## androiddeployqt App:
#####################################################################

qt_internal_add_app(androiddeployqt
    SOURCES
        main.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
        QT_NO_FOREACH
    PUBLIC_LIBRARIES
        Qt::Core # special case
)
set_target_properties(androiddeployqt PROPERTIES
    WIN32_EXECUTABLE FALSE
)

#### Keys ignored in scope 1:.:.:androiddeployqt.pro:<TRUE>:
# _OPTION = "host_build"

## Scopes:
#####################################################################

qt_extend_target(androiddeployqt CONDITION windows
    COMPILE_OPTIONS
        -U__STRICT_ANSI__
)
