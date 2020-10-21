# special case begin
# special case skip regeneration
if (NOT WIN32)
    return()
endif()

# The EntryPoint package consists of two targets: one for CMake consumption,
# and one internal that produces the static library. Together these form the
# entrypoint module in qmake terms. This split allows us to inject library
# dependencies that need to go _before_ the static library, to work around
# CMake's lack of whole archive.

# ---- The header-only target produces the actual module ----
qt_internal_add_module(EntryPoint
    HEADER_MODULE
    INTERNAL_MODULE
    NO_SYNC_QT
    NO_MODULE_HEADERS
    NO_PRIVATE_MODULE
)

# We don't need any include paths or default module defines
set_target_properties(EntryPoint PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ""
    INTERFACE_COMPILE_DEFINITIONS ""
)

# And since this module is the one producing the module pri,
# we need to manually tell it that what we're actually doing
# is producing a module that has a static library.
set_target_properties(EntryPoint PROPERTIES
    INTERFACE_QT_MODULE_INTERNAL_CONFIG "staticlib"
)


# ---- While the static library target does the work ----
qt_internal_add_cmake_library(EntryPointImplementation STATIC
    INCLUDE_DIRECTORIES
       $<TARGET_PROPERTY:Qt::Core,INTERFACE_INCLUDE_DIRECTORIES>
)

set_target_properties(EntryPointImplementation PROPERTIES
    OUTPUT_NAME "${INSTALL_CMAKE_NAMESPACE}EntryPoint${QT_LIBINFIX}"
    ARCHIVE_OUTPUT_DIRECTORY "${QT_BUILD_DIR}/${INSTALL_LIBDIR}"
)

# ---- Now we're ready to set up the platform specifics ----

if(WIN32)
    qt_internal_extend_target(EntryPointImplementation
        SOURCES qtentrypoint_win.cpp
        LIBRARIES shell32
    )

    if(MSVC)
        # Store debug information inside the static lib
        qt_internal_replace_compiler_flags(
            "/Zi" "/Z7"
            CONFIGS DEBUG RELWITHDEBINFO
            IN_CURRENT_SCOPE)
    endif()

    if(MINGW)
        # The mingw32 library needs to come before the entry-point library in the
        # linker line, so that the static linker will pick up the WinMain symbol
        # from the entry-point library. The logic is duplicated in entrypoint.prf
        # on the qmake side.
        target_link_libraries(EntryPoint INTERFACE mingw32)
        target_compile_definitions(EntryPoint INTERFACE QT_NEEDS_QMAIN)
        qt_internal_extend_target(EntryPointImplementation DEFINES QT_NEEDS_QMAIN)
    endif()
endif()

# ---- Finally, make sure the static library can be consumed by clients -----

# Must be added last, so that any library dependencies added above will
# precede the entrypoint library at link time.
target_link_libraries(EntryPoint INTERFACE EntryPointImplementation)

set(export_name "${INSTALL_CMAKE_NAMESPACE}EntryPointTargets")
qt_install(TARGETS EntryPointImplementation EXPORT ${export_name})
qt_generate_prl_file(EntryPointImplementation "${INSTALL_LIBDIR}")

# special case end
