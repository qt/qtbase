# Generated from plugin1.pro.

#####################################################################
## plugin1 Generic Library:
#####################################################################

add_cmake_library(plugin1
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

#### Keys ignored in scope 6:.:..:../winrt.pri:(CMAKE_BUILD_TYPE STREQUAL Debug):
# DESTDIR = "../debug/bin"

#### Keys ignored in scope 7:.:..:../winrt.pri:else:
# DESTDIR = "../release/bin"

extend_target(plugin1 CONDITION NOT QT_FEATURE_library
    DEFINES
        QT_STATICPLUGIN
)

qt_autogen_tools_initial_setup(plugin1)
