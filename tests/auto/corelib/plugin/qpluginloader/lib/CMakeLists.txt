# Generated from lib.pro.

#####################################################################
## tst_qpluginloaderlib Generic Library:
#####################################################################

qt_internal_add_cmake_library(tst_qpluginloaderlib
    SHARED
    INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/tst_qpluginloader/bin"
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bin"
    SOURCES
        mylib.c
    PUBLIC_LIBRARIES
        Qt::Core
)

# special case begin
if(WIN32)
    # CMake sets for Windows-GNU platforms the suffix "lib"
    set_property(TARGET tst_qpluginloaderlib PROPERTY PREFIX "")
endif()
# special case begin

#### Keys ignored in scope 1:.:.:lib.pro:<TRUE>:
# INSTALLS = "target"
# TEMPLATE = "lib"
# target.path = "$$[QT_INSTALL_TESTS]/tst_qpluginloader/bin"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qpluginloaderlib CONDITION MSVC
    DEFINES
        WIN32_MSVC
)

# special case begin
set_target_properties(tst_qpluginloaderlib PROPERTIES
    C_VISIBILITY_PRESET "default"
    CXX_VISIBILITY_PRESET "default"
)
# special case end
