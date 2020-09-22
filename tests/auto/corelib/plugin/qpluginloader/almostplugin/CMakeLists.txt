# Generated from almostplugin.pro.

#####################################################################
## almostplugin Generic Library:
#####################################################################

qt_internal_add_cmake_library(almostplugin
    MODULE
    INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/tst_qpluginloader/bin"
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bin"
    SOURCES
        almostplugin.cpp almostplugin.h
    PUBLIC_LIBRARIES
        Qt::Core
)

#### Keys ignored in scope 1:.:.:almostplugin.pro:<TRUE>:
# INSTALLS = "target"
# TEMPLATE = "lib"
# target.path = "$$[QT_INSTALL_TESTS]/tst_qpluginloader/bin"

## Scopes:
#####################################################################

qt_autogen_tools_initial_setup(almostplugin)
# special case begin
# We want the opposite of this
#qt_internal_extend_target(almostplugin CONDITION GCC
#    LINK_OPTIONS
#        "--Wl,--no-undefined"
#)
# special case end
