include(QtRunCMake)

set(cmake_opts "-DQt6_DIR=${Qt6_DIR}")

# Depending on which module features were used when building Qt, look up each of the respective
# packages.
if(HAS_GUI)
    list(APPEND cmake_opts "-DHAS_GUI=TRUE")
endif()
if(HAS_DBUS)
    list(APPEND cmake_opts "-DHAS_DBUS=TRUE")
endif()
if(HAS_WIDGETS)
    list(APPEND cmake_opts "-DHAS_WIDGETS=TRUE")
endif()
if(HAS_OPENGL)
    list(APPEND cmake_opts "-DHAS_OPENGL=TRUE")
endif()
run_cmake_with_options(project ${cmake_opts})
