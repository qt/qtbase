# Set up some internal requirements for the Startup target.
#
# The creation of the Startup target and its linkage setup happens in 2 places:
# - in src/corelib/CMakeLists.txt when building qtbase.
# - at find_package(Qt6Core) time.
#
# See _qt_internal_setup_startup_target() in Qt6CoreMacros.cmake for the implementation of that.
function(qt_internal_setup_startup_target)
    set(dependent_target "Core")

    # On windows, find_package(Qt6Core) should call find_package(Qt6WinMain) so that Startup can
    # link against WinMain.
    if(WIN32)
        qt_record_extra_qt_package_dependency("${dependent_target}" WinMain "${PROJECT_VERSION}")
    endif()
endfunction()
