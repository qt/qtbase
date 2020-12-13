# special case begin
# special case skip regeneration
if (NOT WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
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

if(WIN32)
    # Not all platforms require the static library
    set(using_entrypoint_library "yes")
endif()

if(using_entrypoint_library)
    # ---- While the static library target does the work ----
    qt_internal_add_cmake_library(EntryPointImplementation STATIC
        INCLUDE_DIRECTORIES
           $<TARGET_PROPERTY:Qt::Core,INTERFACE_INCLUDE_DIRECTORIES>
    )

    set_target_properties(EntryPointImplementation PROPERTIES
        OUTPUT_NAME "${INSTALL_CMAKE_NAMESPACE}EntryPoint${QT_LIBINFIX}"
        ARCHIVE_OUTPUT_DIRECTORY "${QT_BUILD_DIR}/${INSTALL_LIBDIR}"
    )

    qt_handle_multi_config_output_dirs(EntryPointImplementation)
    qt_internal_add_target_aliases(EntryPointImplementation)
    qt_set_common_target_properties(EntryPointImplementation)
endif()

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
        # from the entry-point library.
        target_link_libraries(EntryPoint INTERFACE mingw32)
        set_property(TARGET EntryPoint APPEND PROPERTY INTERFACE_QT_MODULE_LDFLAGS "-lmingw32")

        target_compile_definitions(EntryPoint INTERFACE QT_NEEDS_QMAIN)
        qt_internal_extend_target(EntryPointImplementation DEFINES QT_NEEDS_QMAIN)
    endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set_target_properties(EntryPoint PROPERTIES INTERFACE_LINK_OPTIONS "-Wl,-e,_qt_main_wrapper")
    set_property(TARGET EntryPoint APPEND PROPERTY INTERFACE_QT_MODULE_LDFLAGS "-Wl,-e,_qt_main_wrapper")
endif()

# ---- Finally, make sure the static library can be consumed by clients -----

if(using_entrypoint_library)
    target_link_libraries(EntryPoint INTERFACE Qt6::EntryPointImplementation)

    qt_internal_get_target_property(entrypoint_implementation_ldflags
        EntryPointImplementation QT_MODULE_LDFLAGS)

    set_target_properties(EntryPoint PROPERTIES
        INTERFACE_QT_MODULE_PRI_EXTRA_CONTENT "
QT.entrypoint_implementation.name = QtEntryPointImplementation
QT.entrypoint_implementation.module = Qt6EntryPoint
QT.entrypoint_implementation.ldflags = ${entrypoint_implementation_ldflags}
QT.entrypoint_implementation.libs = $$QT_MODULE_LIB_BASE
QT.entrypoint_implementation.module_config = staticlib v2 internal_module
"
        INTERFACE_QT_MODULE_DEPENDS "entrypoint_implementation"
    )

    set(export_name "${INSTALL_CMAKE_NAMESPACE}EntryPointTargets")
    qt_install(TARGETS EntryPointImplementation EXPORT ${export_name})
    qt_generate_prl_file(EntryPointImplementation "${INSTALL_LIBDIR}")
endif()

# special case end
