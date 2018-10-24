add_qt_tool(uic
    SOURCES
        cpp/cppwritedeclaration.cpp
        cpp/cppwriteincludes.cpp
        cpp/cppwriteinitialization.cpp
        customwidgetsinfo.cpp
        databaseinfo.cpp
        driver.cpp
        main.cpp
        treewalker.cpp
        ui4.cpp
        uic.cpp
        validator.cpp
    INCLUDE_DIRECTORIES cpp
    DEFINES QT_UIC QT_UIC_CPP_GENERATOR
)
