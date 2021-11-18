#####################################################################
## windeployqt Tool:
#####################################################################

qt_get_tool_target_name(target_name windeployqt)
qt_internal_add_tool(${target_name}
    TOOLS_TARGET Core
    USER_FACING
    TARGET_DESCRIPTION "Qt Windows Deployment Tool"
    SOURCES
        elfreader.cpp elfreader.h
        qmlutils.cpp qmlutils.h
        utils.cpp utils.h
        main.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
        QT_NO_FOREACH
    LIBRARIES
        Qt::CorePrivate
)
qt_internal_return_unless_building_tools()

qt_internal_extend_target(${target_name} CONDITION WIN32
    PUBLIC_LIBRARIES
        shlwapi
)

qt_internal_extend_target(${target_name} CONDITION QT_FEATURE_relocatable
    DEFINES
        QT_RELOCATABLE
)
