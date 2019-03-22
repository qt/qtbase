# Generated from uic.pro.

#####################################################################
## uic Tool:
#####################################################################

add_qt_tool(uic
    SOURCES
        cpp/cppwritedeclaration.cpp cpp/cppwritedeclaration.h
        cpp/cppwriteincludes.cpp cpp/cppwriteincludes.h
        cpp/cppwriteinitialization.cpp cpp/cppwriteinitialization.h
        customwidgetsinfo.cpp customwidgetsinfo.h
        databaseinfo.cpp databaseinfo.h
        driver.cpp driver.h
        globaldefs.h
        main.cpp
        option.h
        treewalker.cpp treewalker.h
        ui4.cpp ui4.h
        uic.cpp uic.h
        utils.h
        validator.cpp validator.h
    DEFINES
        QT_UIC
        QT_NO_CAST_FROM_ASCII
        QT_NO_FOREACH
        QT_UIC_CPP_GENERATOR
    INCLUDE_DIRECTORIES
        cpp
    DISABLE_AUTOGEN_TOOLS # special case:
        uic rcc
    # QMAKE_TARGET_DESCRIPTION = "Qt User Interface Compiler"
    # _LOADED = "qt_tool"
    # _OPTION = "host_build"
)

## Scopes:
#####################################################################
#
#extend_target(uic CONDITION NOT force_bootstrap AND NOT qtConfig(commandlineparser OR NOT QT_FEATURE_textcodec OR NOT QT_FEATURE_xmlstreamreader OR NOT QT_FEATURE_xmlstreamwriter
#    # CONFIG = "force_bootstrap"
#)
