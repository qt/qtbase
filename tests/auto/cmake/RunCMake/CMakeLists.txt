# Add RunCMake tests using `add_RunCMake_test()`

add_RunCMake_test(QtFlagHandlingHelpers "-DQt6_DIR=${Qt6_DIR}")

set(extra_run_cmake_args "-DQt6_DIR=${Qt6_DIR}")
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
add_RunCMake_test(Qt6DirConfiguration ${extra_run_cmake_args})

if(APPLE AND TARGET Qt::Gui)
    add_RunCMake_test(AppleFrameworkDeduplication "-DQt6_DIR=${Qt6_DIR}")
endif()
