# Generated from plugin2.pro.

#####################################################################
## plugin2 Generic Library:
#####################################################################

qt_internal_add_cmake_library(plugin2
    MODULE
    INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/tst_qfactoryloader/bin"
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bin"
    SOURCES
        plugin2.cpp plugin2.h
    PUBLIC_LIBRARIES
        Qt::Core
)

#### Keys ignored in scope 1:.:.:plugin2.pro:<TRUE>:
# INSTALLS = "target"
# TEMPLATE = "lib"
# target.path = "$$[QT_INSTALL_TESTS]/tst_qfactoryloader/bin"

## Scopes:
#####################################################################

qt_internal_extend_target(plugin2 CONDITION NOT QT_FEATURE_library
    DEFINES
        QT_STATICPLUGIN
)

qt_autogen_tools_initial_setup(plugin2)
