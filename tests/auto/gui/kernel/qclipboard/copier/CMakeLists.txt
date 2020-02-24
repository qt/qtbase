# Generated from copier.pro.

#####################################################################
## copier Binary:
#####################################################################

# special case begin
set(win32_args)
if (WIN32)
    set(win32_args OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
endif()
# special case end

add_qt_executable(copier
    ${win32_args} # special case
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:copier.pro:WIN32:
# DESTDIR = "../copier"
