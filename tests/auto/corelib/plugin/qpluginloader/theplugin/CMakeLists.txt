# Generated from theplugin.pro.

#####################################################################
## theplugin Plugin:
#####################################################################

add_qt_plugin(theplugin
    INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/tst_qpluginloader/bin"
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bin"
    SOURCES
        theplugin.cpp theplugin.h
    PUBLIC_LIBRARIES
        Qt::Core
)

#### Keys ignored in scope 1:.:.:theplugin.pro:<TRUE>:
# INSTALLS = "target"
# TEMPLATE = "lib"
# target.path = "$$[QT_INSTALL_TESTS]/tst_qpluginloader/bin"

## Scopes:
#####################################################################

#### Keys ignored in scope 5:.:..:../winrt.pri:(CMAKE_BUILD_TYPE STREQUAL Debug):
# DESTDIR = "../debug/bin"

#### Keys ignored in scope 6:.:..:../winrt.pri:else:
# DESTDIR = "../release/bin"
