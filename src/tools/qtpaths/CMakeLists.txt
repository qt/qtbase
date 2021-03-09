# Generated from qtpaths.pro.

#####################################################################
## qtpaths App:
#####################################################################

qt_get_tool_target_name(target_name qtpaths)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "Qt tool that provides the standard paths of the Qt framework"
    TOOLS_TARGET Core
    SOURCES
        qtpaths.cpp
    DEFINES
        QT_NO_FOREACH
)

## Scopes:
#####################################################################

if(WIN32)
    set_target_properties(qtpaths PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
endif()
