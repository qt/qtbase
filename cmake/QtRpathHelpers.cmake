function(qt_compute_relative_rpath_base rpath install_location out_var)
    set(install_lib_dir_absolute "${CMAKE_INSTALL_PREFIX}/${INSTALL_LIBDIR}")
    get_filename_component(rpath_absolute "${rpath}"
                           ABSOLUTE BASE_DIR "${install_lib_dir_absolute}")

   if(NOT IS_ABSOLUTE)
       set(install_location_absolute "${CMAKE_INSTALL_PREFIX}/${install_location}")
   endif()
    # Compute relative rpath from where the target will be installed, to the place where libraries
    # will be placed (INSTALL_LIBDIR).
    file(RELATIVE_PATH rpath_relative "${install_location_absolute}" "${rpath_absolute}")

    if("${rpath_relative}" STREQUAL "")
        # file(RELATIVE_PATH) returns an empty string if the given absolute paths are equal
        set(rpath_relative ".")
    endif()

    # Prepend $ORIGIN / @loader_path style tokens (qmake's QMAKE_REL_RPATH_BASE), to make the
    # relative rpaths work. qmake does this automatically when generating a project, so it wasn't
    # needed in the .prf files, but for CMake we need to prepend them ourselves.
    if(APPLE)
        set(rpath_rel_base "@loader_path")
    elseif(LINUX OR SOLARIS OR FREEBSD)
        set(rpath_rel_base "$ORIGIN")
    else()
        message(WARNING "No known RPATH_REL_BASE for target platform.")
        set(rpath_rel_base "NO_KNOWN_RPATH_REL_BASE")
    endif()

    if(rpath_relative STREQUAL ".")
        set(rpath_relative "${rpath_rel_base}")
    else()
        set(rpath_relative "${rpath_rel_base}/${rpath_relative}")
    endif()

    set("${out_var}" "${rpath_relative}" PARENT_SCOPE)
endfunction()

# Applies necessary rpaths to a target upon target installation.
# No-op when targeting Windows, Android, or non-prefix builds.
#
# If no RELATIVE_RPATH option is given, embeds an absolute path rpath to ${INSTALL_LIBDIR}.
# If RELATIVE_RPATH is given, the INSTALL_PATH value is to compute the relative path from
# ${INSTALL_LIBDIR} to wherever the target will be installed (the value of INSTALL_PATH).
# It's the equivalent of qmake's relative_qt_rpath.
# INSTALL_PATH is used to implement the equivalent of qmake's $$qtRelativeRPathBase().
#
# A cache variable QT_DISABLE_RPATH can be set to disable embedding any rpaths when installing.
function(qt_apply_rpaths)
    # No rpath support for win32 and android. Also no need to apply rpaths when doing a non-prefix
    # build.
    if(WIN32 OR ANDROID)
        return()
    endif()

    # Rpaths xplicitly disabled (like for uikit), equivalent to qmake's no_qt_rpath.
    if(QT_DISABLE_RPATH)
        return()
    endif()

    qt_parse_all_arguments(arg "qt_apply_rpaths" "RELATIVE_RPATH" "TARGET;INSTALL_PATH" "" ${ARGN})
    if(NOT arg_TARGET)
        message(FATAL_ERROR "No target given to qt_apply_rpaths.")
    else()
        set(target "${arg_TARGET}")
    endif()

    # If a target is not built (which can happen for tools when crosscompiling, we shouldn't try
    # to apply properties.
    if(NOT TARGET "${target}")
        return()
    endif()

    # Protect against interface libraries.
    get_target_property(target_type "${target}" TYPE)
    if (target_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()

    if(NOT arg_INSTALL_PATH)
        message(FATAL_ERROR "No INSTALL_PATH given to qt_apply_rpaths.")
    endif()

    set(rpaths "")

    # Modify the install path to contain the nested structure of a framework.
    get_target_property(is_framework "${target}" FRAMEWORK)
    if(is_framework)
        qt_internal_get_framework_info(fw ${target})
        if(UIKIT)
            # Shallow framework
            string(APPEND arg_INSTALL_PATH "/${fw_dir}")
        else()
            # Full framework
            string(APPEND arg_INSTALL_PATH "/${fw_dir}/Versions/Current")
        endif()
    endif()

    # Same but for an app bundle.
    get_target_property(is_bundle "${target}" MACOSX_BUNDLE)
    if(is_bundle AND NOT is_framework)
        if(UIKIT)
            # Shallow bundle
            string(APPEND arg_INSTALL_PATH "/${target}.app")
        else()
            # Full bundle
            string(APPEND arg_INSTALL_PATH "/${target}.app/Contents/MacOS")
        endif()
    endif()

    # Somewhat similar to mkspecs/features/qt.prf
    if(arg_RELATIVE_RPATH)
        qt_compute_relative_rpath_base(
            "${_default_install_rpath}" "${arg_INSTALL_PATH}" relative_rpath)
        list(APPEND rpaths "${relative_rpath}")
    else()
        list(APPEND rpaths "${_default_install_rpath}")
    endif()

    # Somewhat similar to mkspecs/features/qt_build_extra.prf.
    foreach(rpath ${QT_EXTRA_RPATHS})
        if(IS_ABSOLUTE)
            list(APPEND rpaths "${rpath}")
        else()
            qt_compute_relative_rpath_base("${rpath}" "${arg_INSTALL_PATH}" relative_rpath)
            list(APPEND rpaths "${relative_rpath}")
        endif()
    endforeach()

    if(rpaths)
        list(REMOVE_DUPLICATES rpaths)
        if(QT_WILL_INSTALL)
            set(prop_name "INSTALL_RPATH")
        else()
            set(prop_name "BUILD_RPATH")
        endif()
        set_property(TARGET "${target}" APPEND PROPERTY "${prop_name}" ${rpaths})
    endif()
endfunction()
