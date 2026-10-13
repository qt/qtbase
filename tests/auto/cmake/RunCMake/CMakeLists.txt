# Add RunCMake tests using `add_RunCMake_test()`

qt_internal_add_RunCMake_test(QtFlagHandlingHelpers)

set(extra_run_cmake_args "")
if(TARGET Qt::Gui)
    list(APPEND extra_run_cmake_args "-DHAS_GUI=TRUE")
endif()
if(TARGET Qt::DBus)
    list(APPEND extra_run_cmake_args "-DHAS_DBUS=TRUE")
endif()
if(TARGET Qt::Widgets)
    list(APPEND extra_run_cmake_args "-DHAS_WIDGETS=TRUE")
endif()
if(TARGET Qt::OpenGL)
    list(APPEND extra_run_cmake_args "-DHAS_OPENGL=TRUE")
endif()
qt_internal_add_RunCMake_test(Qt6DirConfiguration ${extra_run_cmake_args})

if(APPLE AND TARGET Qt::Gui)
    qt_internal_add_RunCMake_test(AppleFrameworkDeduplication)
endif()

if(TARGET Qt6::Platform)
    get_target_property(sbom_spdx_id Qt6::Platform _qt_sbom_spdx_id)
    # If the id is present and non-empty, that means we built Qt with sbom support.
    if(sbom_spdx_id)
        qt_internal_add_RunCMake_test(Sbom ${extra_run_cmake_args})
    endif()
endif()
