# Generated from paster.pro.

#####################################################################
## paster Binary:
#####################################################################

# special case begin
set(win32_args)
if (WIN32)
    set(win32_args OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
endif()
# special case end

add_qt_executable(paster
    ${win32_args} # special case
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:paster.pro:WIN32:
# DESTDIR = "../paster"
