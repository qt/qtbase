# Generated from copier.pro.

#####################################################################
## copier Binary:
#####################################################################

# special case begin
set(args OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
# special case end

qt_internal_add_executable(copier
    ${args} # special case
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:copier.pro:WIN32:
# DESTDIR = "../copier"
