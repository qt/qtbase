# Generated from rcc.pro.

#####################################################################
## rcc Tool:
#####################################################################

qt_get_tool_target_name(target_name rcc)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "Qt Resource Compiler"
    INSTALL_DIR "${INSTALL_LIBEXECDIR}"
    TOOLS_TARGET Core # special case
    SOURCES
        main.cpp
        rcc.cpp rcc.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_FOREACH
        QT_RCC
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
)
qt_internal_return_unless_building_tools()

#### Keys ignored in scope 1:.:.:rcc.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "Qt Resource Compiler"
# _OPTION = "host_build"

## Scopes:
#####################################################################

qt_internal_extend_target(${target_name} CONDITION QT_FEATURE_zstd
    LIBRARIES
        WrapZSTD::WrapZSTD
)
