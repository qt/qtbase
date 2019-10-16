# Generated from plugin2.pro.

#####################################################################
## plugin2 Plugin:
#####################################################################

add_qt_plugin(plugin2
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

#### Keys ignored in scope 6:.:..:../winrt.pri:(CMAKE_BUILD_TYPE STREQUAL Debug):
# DESTDIR = "../debug/bin"

#### Keys ignored in scope 7:.:..:../winrt.pri:else:
# DESTDIR = "../release/bin"

extend_target(plugin2 CONDITION NOT QT_FEATURE_library
    DEFINES
        QT_STATICPLUGIN
)
