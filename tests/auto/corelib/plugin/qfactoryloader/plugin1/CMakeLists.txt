# Generated from plugin1.pro.

#####################################################################
## plugin1 Generic Library:
#####################################################################

qt_internal_add_cmake_library(plugin1
    MODULE
    INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/tst_qfactoryloader/bin"
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bin"
    SOURCES
        plugin1.cpp plugin1.h
    PUBLIC_LIBRARIES
        Qt::Core
)

#### Keys ignored in scope 1:.:.:plugin1.pro:<TRUE>:
# INSTALLS = "target"
# TEMPLATE = "lib"
# target.path = "$$[QT_INSTALL_TESTS]/tst_qfactoryloader/bin"

## Scopes:
#####################################################################

qt_internal_extend_target(plugin1 CONDITION NOT QT_FEATURE_library
    DEFINES
        QT_STATICPLUGIN
)

qt_autogen_tools_initial_setup(plugin1)
