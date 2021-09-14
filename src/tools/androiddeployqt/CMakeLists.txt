# Generated from androiddeployqt.pro.

#####################################################################
## androiddeployqt App:
#####################################################################

qt_get_tool_target_name(target_name androiddeployqt)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "Qt Android Deployment Tool"
    TOOLS_TARGET Core
    SOURCES
        main.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
        QT_NO_FOREACH
    PUBLIC_LIBRARIES
        Qt::Core # special case
    INCLUDE_DIRECTORIES
        ../shared
)
qt_internal_return_unless_building_tools()
set_target_properties(${target_name} PROPERTIES
    WIN32_EXECUTABLE FALSE
)

#### Keys ignored in scope 1:.:.:androiddeployqt.pro:<TRUE>:
# _OPTION = "host_build"

## Scopes:
#####################################################################

qt_internal_extend_target(${target_name} CONDITION windows
    COMPILE_OPTIONS
        -U__STRICT_ANSI__
)
