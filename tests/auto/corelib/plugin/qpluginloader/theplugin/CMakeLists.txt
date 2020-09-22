# Generated from theplugin.pro.

#####################################################################
## theplugin Generic Library:
#####################################################################

qt_internal_add_cmake_library(theplugin
    MODULE
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

qt_autogen_tools_initial_setup(theplugin)
