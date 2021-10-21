# This function recursively walks transitive link libraries of the given target
# and promotes those targets to be IMPORTED_GLOBAL if they are not.
#
# This is required for .prl file generation in top-level builds, to make sure that imported 3rd
# party library targets in any repo are made global, so there are no scoping issues.
#
# Only works if called from qt_find_package(), because the promotion needs to happen in the same
# directory scope where the imported target is first created.
#
# Uses __qt_internal_walk_libs.
function(qt_find_package_promote_targets_to_global_scope target)
    __qt_internal_walk_libs("${target}" _discarded_out_var _discarded_out_var_2
                            "qt_find_package_targets_dict" "promote_global")
endfunction()

macro(qt_find_package)
    # Get the target names we expect to be provided by the package.
    set(find_package_options CONFIG NO_MODULE MODULE REQUIRED)
    set(options ${find_package_options} MARK_OPTIONAL)
    set(oneValueArgs MODULE_NAME QMAKE_LIB)
    set(multiValueArgs PROVIDED_TARGETS COMPONENTS OPTIONAL_COMPONENTS)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # If some Qt internal project calls qt_find_package(WrapFreeType), but WrapFreeType was already
    # found as part of a find_dependency() call from a ModuleDependencies.cmake file (or similar),
    # and the provided target is also found, that means this might have been an unnecessary
    # qt_find_package() call, because the dependency was already found via some other transitive
    # dependency. Return early, so that CMake doesn't fail wiht an error with trying to promote the
    # targets to be global. This behavior is not enabled by default, because there are cases
    # when a regular find_package() (non qt_) can find a package (Freetype -> PNG), and a subsequent
    # qt_find_package(PNG PROVIDED_TARGET PNG::PNG) still needs to succeed and register the provided
    # targets. To enable the debugging behavior, set QT_DEBUG_QT_FIND_PACKAGE to 1.
    set(_qt_find_package_skip_find_package FALSE)
    if(QT_DEBUG_QT_FIND_PACKAGE AND ${ARGV0}_FOUND AND arg_PROVIDED_TARGETS)
        set(_qt_find_package_skip_find_package TRUE)
        foreach(qt_find_package_target_name ${arg_PROVIDED_TARGETS})
            if(NOT TARGET ${qt_find_package_target_name})
                set(_qt_find_package_skip_find_package FALSE)
            endif()
        endforeach()

        if(_qt_find_package_skip_find_package)
            message(AUTHOR_WARNING "qt_find_package(${ARGV0}) called even though the package "
                   "was already found. Consider removing the call.")
        endif()
    endif()

    # When configure.cmake is included only to record summary entries, there's no point in looking
    # for the packages.
    if(__QtFeature_only_record_summary_entries)
        set(_qt_find_package_skip_find_package TRUE)
    endif()

    # Get the version if specified.
    set(package_version "")
    if(${ARGC} GREATER_EQUAL 2)
        if(${ARGV1} MATCHES "^[0-9\.]+$")
            set(package_version "${ARGV1}")
        endif()
    endif()

    if(arg_COMPONENTS)
        # Re-append components to forward them.
        list(APPEND arg_UNPARSED_ARGUMENTS "COMPONENTS;${arg_COMPONENTS}")
    endif()
    if(arg_OPTIONAL_COMPONENTS)
        # Re-append optional components to forward them.
        list(APPEND arg_UNPARSED_ARGUMENTS "OPTIONAL_COMPONENTS;${arg_OPTIONAL_COMPONENTS}")
    endif()

    # Don't look for packages in PATH if requested to.
    if(QT_NO_USE_FIND_PACKAGE_SYSTEM_ENVIRONMENT_PATH)
        set(_qt_find_package_use_system_env_backup "${CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH}")
        set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH "OFF")
    endif()

    if(NOT (arg_CONFIG OR arg_NO_MODULE OR arg_MODULE) AND NOT _qt_find_package_skip_find_package)
        # Try to find a config package first in quiet mode
        set(config_package_arg ${arg_UNPARSED_ARGUMENTS})
        list(APPEND config_package_arg "CONFIG;QUIET")
        find_package(${config_package_arg})

        # Double check that in config mode the targets become visible. Sometimes
        # only the module mode creates the targets. For example with vcpkg, the sqlite
        # package provides sqlite3-config.cmake, which offers multi-config targets but
        # in their own way. CMake has FindSQLite3.cmake and with the original
        # qt_find_package(SQLite3) call it is our intention to use the cmake package
        # in module mode.
        unset(_qt_any_target_found)
        unset(_qt_should_unset_found_var)
        if(${ARGV0}_FOUND AND arg_PROVIDED_TARGETS)
            foreach(expected_target ${arg_PROVIDED_TARGETS})
                if (TARGET ${expected_target})
                    set(_qt_any_target_found TRUE)
                    break()
                endif()
            endforeach()
            if(NOT _qt_any_target_found)
                set(_qt_should_unset_found_var TRUE)
            endif()
        endif()
        # If we consider the package not to be found, make sure to unset both regular
        # and CACHE vars, otherwise CMP0126 set to NEW might cause issues with
        # packages not being found correctly.
        if(NOT ${ARGV0}_FOUND OR _qt_should_unset_found_var)
            unset(${ARGV0}_FOUND)
            unset(${ARGV0}_FOUND CACHE)

            # Unset the NOTFOUND ${package}_DIR var that might have been set by the previous
            # find_package call, to get rid of "not found" messages in the feature summary
            # if the package is found by the next find_package call.
            if(DEFINED CACHE{${ARGV0}_DIR} AND NOT ${ARGV0}_DIR)
                unset(${ARGV0}_DIR CACHE)
            endif()
        endif()
    endif()

    # Ensure the options are back in the original unparsed arguments
    foreach(opt IN LISTS find_package_options)
        if(arg_${opt})
            list(APPEND arg_UNPARSED_ARGUMENTS ${opt})
        endif()
    endforeach()

    # TODO: Handle packages with components where a previous component is already found.
    # E.g. find_package(Qt6 COMPONENTS BuildInternals) followed by
    # qt_find_package(Qt6 COMPONENTS Core) doesn't end up calling find_package(Qt6Core).
    if (NOT ${ARGV0}_FOUND AND NOT _qt_find_package_skip_find_package)
        # Call original function without our custom arguments.
        find_package(${arg_UNPARSED_ARGUMENTS})
    endif()

    if(QT_NO_USE_FIND_PACKAGE_SYSTEM_ENVIRONMENT_PATH)
        if("${_qt_find_package_use_system_env_backup}" STREQUAL "")
            unset(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH)
        else()
            set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH "${_qt_find_package_use_system_env_backup}")
        endif()
    endif()

    if(${ARGV0}_FOUND AND arg_PROVIDED_TARGETS AND NOT _qt_find_package_skip_find_package)
        # If package was found, associate each target with its package name. This will be used
        # later when creating Config files for Qt libraries, to generate correct find_dependency()
        # calls. Also make the provided targets global, so that the properties can be read in
        # all scopes.
        foreach(qt_find_package_target_name ${arg_PROVIDED_TARGETS})
            if(TARGET ${qt_find_package_target_name})
                # Allow usage of aliased targets by setting properties on the actual target
                get_target_property(aliased_target ${qt_find_package_target_name} ALIASED_TARGET)
                if(aliased_target)
                    set(qt_find_package_target_name ${aliased_target})
                endif()

                set_target_properties(${qt_find_package_target_name} PROPERTIES
                    INTERFACE_QT_PACKAGE_NAME ${ARGV0}
                    INTERFACE_QT_PACKAGE_IS_OPTIONAL ${arg_MARK_OPTIONAL})
                if(package_version)
                    set_target_properties(${qt_find_package_target_name}
                                          PROPERTIES INTERFACE_QT_PACKAGE_VERSION ${ARGV1})
                endif()

                if(arg_COMPONENTS)
                    string(REPLACE ";" " " components_as_string "${arg_COMPONENTS}")
                    set_property(TARGET ${qt_find_package_target_name}
                                 PROPERTY INTERFACE_QT_PACKAGE_COMPONENTS ${components_as_string})
                endif()

                if(arg_OPTIONAL_COMPONENTS)
                    string(REPLACE ";" " " components_as_string "${arg_OPTIONAL_COMPONENTS}")
                    set_property(TARGET ${qt_find_package_target_name}
                                 PROPERTY INTERFACE_QT_PACKAGE_OPTIONAL_COMPONENTS
                                 ${components_as_string})
                endif()

                get_property(is_global TARGET ${qt_find_package_target_name} PROPERTY
                                                                             IMPORTED_GLOBAL)
                qt_internal_should_not_promote_package_target_to_global(
                    "${qt_find_package_target_name}" should_not_promote)
                if(NOT is_global AND NOT should_not_promote)
                    __qt_internal_promote_target_to_global(${qt_find_package_target_name})
                    qt_find_package_promote_targets_to_global_scope(
                        "${qt_find_package_target_name}")
                endif()
            endif()

        endforeach()

        if(arg_MODULE_NAME AND arg_QMAKE_LIB
           AND (NOT arg_QMAKE_LIB IN_LIST QT_QMAKE_LIBS_FOR_${arg_MODULE_NAME}))
            set(QT_QMAKE_LIBS_FOR_${arg_MODULE_NAME}
                ${QT_QMAKE_LIBS_FOR_${arg_MODULE_NAME}};${arg_QMAKE_LIB} CACHE INTERNAL "")
            set(QT_TARGETS_OF_QMAKE_LIB_${arg_QMAKE_LIB} ${arg_PROVIDED_TARGETS} CACHE INTERNAL "")
            foreach(provided_target ${arg_PROVIDED_TARGETS})
                set(QT_QMAKE_LIB_OF_TARGET_${provided_target} ${arg_QMAKE_LIB} CACHE INTERNAL "")
            endforeach()
        endif()
    endif()
endmacro()

# Return qmake library name for the given target, e.g. return "vulkan" for "Vulkan::Vulkan".
function(qt_internal_map_target_to_qmake_lib target out_var)
    set(${out_var} "${QT_QMAKE_LIB_OF_TARGET_${target}}" PARENT_SCOPE)
endfunction()

# This function records a dependency between ${main_target_name} and ${dep_package_name}.
# at the CMake package level.
# E.g. The Tools package that provides the qtwaylandscanner target
# needs to call find_package(WaylandScanner) (non-qt-package).
# main_target_name = qtwaylandscanner
# dep_package_name = WaylandScanner
function(qt_record_extra_package_dependency main_target_name dep_package_name dep_package_version)
    if(NOT TARGET "${main_target_name}")
        qt_get_tool_target_name(main_target_name "${main_target_name}")
    endif()
    if (TARGET "${main_target_name}")
        get_target_property(extra_packages "${main_target_name}" QT_EXTRA_PACKAGE_DEPENDENCIES)
        if(NOT extra_packages)
            set(extra_packages "")
        endif()

        list(APPEND extra_packages "${dep_package_name}\;${dep_package_version}")
        set_target_properties("${main_target_name}" PROPERTIES QT_EXTRA_PACKAGE_DEPENDENCIES
                                                               "${extra_packages}")
    endif()
endfunction()

# This function records a dependency between ${main_target_name} and ${dep_target_name}
# at the CMake package level.
# E.g. Qt6CoreConfig.cmake needs to find_package(Qt6EntryPointPrivate).
# main_target_name = Core
# dep_target_name = EntryPointPrivate
# This is just a convenience function that deals with Qt targets and their associated packages
# instead of raw package names.
function(qt_record_extra_qt_package_dependency main_target_name dep_target_name
                                                                dep_package_version)
    # EntryPointPrivate -> Qt6EntryPointPrivate.
    qt_internal_qtfy_target(qtfied_target_name "${dep_target_name}")
    qt_record_extra_package_dependency("${main_target_name}"
        "${qtfied_target_name_versioned}" "${dep_package_version}")
endfunction()

# This function records a 'QtFooTools' package dependency for the ${main_target_name} target
# onto the ${dep_package_name} tools package.
# E.g. The QtWaylandCompositor package needs to call find_package(QtWaylandScannerTools).
# main_target_name = WaylandCompositor
# dep_package_name = Qt6WaylandScannerTools
function(qt_record_extra_main_tools_package_dependency
         main_target_name dep_package_name dep_package_version)
    if(NOT TARGET "${main_target_name}")
        qt_get_tool_target_name(main_target_name "${main_target_name}")
    endif()
    if (TARGET "${main_target_name}")
        get_target_property(extra_packages "${main_target_name}"
                            QT_EXTRA_TOOLS_PACKAGE_DEPENDENCIES)
        if(NOT extra_packages)
            set(extra_packages "")
        endif()

        list(APPEND extra_packages "${dep_package_name}\;${dep_package_version}")
        set_target_properties("${main_target_name}" PROPERTIES QT_EXTRA_TOOLS_PACKAGE_DEPENDENCIES
                                                               "${extra_packages}")
    endif()
endfunction()

# This function records a 'QtFooTools' package dependency for the ${main_target_name} target
# onto the ${dep_non_versioned_package_name} Tools package.
# main_target_name = WaylandCompositor
# dep_non_versioned_package_name = WaylandScannerTools
# This is just a convenience function to avoid hardcoding the qtified version in the dep package
# name.
function(qt_record_extra_qt_main_tools_package_dependency main_target_name
                                                          dep_non_versioned_package_name
                                                          dep_package_version)
    # WaylandScannerTools -> Qt6WaylandScannerTools.
    qt_internal_qtfy_target(qtfied_package_name "${dep_non_versioned_package_name}")
    qt_record_extra_main_tools_package_dependency(
        "${main_target_name}" "${qtfied_package_name_versioned}" "${dep_package_version}")
endfunction()

# Record an extra 3rd party target as a dependency for ${main_target_name}.
#
# Adds a find_package(${dep_target_package_name}) in ${main_target_name}Dependencies.cmake.
#
# Needed to record a dependency on the package that provides WrapVulkanHeaders::WrapVulkanHeaders.
# The package version, components, whether the package is optional, etc, are queried from the
# ${dep_target} target properties.
function(qt_record_extra_third_party_dependency main_target_name dep_target)
    if(NOT TARGET "${main_target_name}")
        qt_get_tool_target_name(main_target_name "${main_target_name}")
    endif()
    if(TARGET "${main_target_name}")
        get_target_property(extra_deps "${main_target_name}" _qt_extra_third_party_dep_targets)
        if(NOT extra_deps)
            set(extra_deps "")
        endif()

        list(APPEND extra_deps "${dep_target}")
        set_target_properties("${main_target_name}" PROPERTIES _qt_extra_third_party_dep_targets
                                                               "${extra_deps}")
    endif()
endfunction()

# Sets out_var to TRUE if the non-namespaced ${lib} target is exported as part of Qt6Targets.cmake.
function(qt_internal_is_lib_part_of_qt6_package lib out_var)
    if (lib STREQUAL "Platform"
            OR lib STREQUAL "GlobalConfig"
            OR lib STREQUAL "GlobalConfigPrivate"
            OR lib STREQUAL "PlatformModuleInternal"
            OR lib STREQUAL "PlatformPluginInternal"
            OR lib STREQUAL "PlatformToolInternal")
        set(${out_var} "TRUE" PARENT_SCOPE)
    else()
        set(${out_var} "FALSE" PARENT_SCOPE)
    endif()
endfunction()

# This function stores the list of Qt targets a library depend on,
# along with their version info, for usage in ${target}Depends.cmake file
function(qt_register_target_dependencies target public_libs private_libs)
    get_target_property(target_deps "${target}" _qt_target_deps)
    if(NOT target_deps)
        set(target_deps "")
    endif()

    get_target_property(target_type ${target} TYPE)
    set(lib_list ${public_libs})

    set(target_is_shared FALSE)
    set(target_is_static FALSE)
    if(target_type STREQUAL "SHARED_LIBRARY")
        set(target_is_shared TRUE)
    elseif(target_type STREQUAL "STATIC_LIBRARY")
        set(target_is_static TRUE)
    endif()

    # Record 'Qt::Foo'-like private dependencies of static library targets, this will be used to
    # generate find_dependency() calls.
    #
    # Private static library dependencies will become $<LINK_ONLY:> dependencies in
    # INTERFACE_LINK_LIBRARIES.
    if(target_is_static)
        list(APPEND lib_list ${private_libs})
    endif()

    foreach(lib IN LISTS lib_list)
        if ("${lib}" MATCHES "^Qt::(.*)")
            set(lib "${CMAKE_MATCH_1}")
            qt_internal_is_lib_part_of_qt6_package("${lib}" is_part_of_qt6)
            if (is_part_of_qt6)
                list(APPEND target_deps "Qt6\;${PROJECT_VERSION}")
            else()
                list(APPEND target_deps "${INSTALL_CMAKE_NAMESPACE}${lib}\;${PROJECT_VERSION}")
            endif()
        endif()
    endforeach()

    # Record 'Qt::Foo'-like shared private dependencies of shared library targets.
    #
    # Private shared library dependencies are listed in the target's
    # IMPORTED_LINK_DEPENDENT_LIBRARIES and used in rpath-link calculation.
    # See QTBUG-86533 for some details.
    # We filter out static libraries and common platform targets, but include both SHARED and
    # INTERFACE libraries. INTERFACE libraries in most cases will be FooPrivate libraries.
    if(target_is_shared AND private_libs)
        foreach(lib IN LISTS private_libs)
            if ("${lib}" MATCHES "^Qt::(.*)")
                set(lib_namespaced "${lib}")
                set(lib "${CMAKE_MATCH_1}")

                qt_internal_is_lib_part_of_qt6_package("${lib}" is_part_of_qt6)
                get_target_property(lib_type "${lib_namespaced}" TYPE)
                if(NOT lib_type STREQUAL "STATIC_LIBRARY" AND NOT is_part_of_qt6)
                    list(APPEND target_deps "${INSTALL_CMAKE_NAMESPACE}${lib}\;${PROJECT_VERSION}")
                endif()
            endif()
        endforeach()
    endif()

    set_target_properties("${target}" PROPERTIES _qt_target_deps "${target_deps}")
endfunction()

# Sets out_var to to TRUE if the target was marked to not be promoted to global scope.
function(qt_internal_should_not_promote_package_target_to_global target out_var)
    get_property(should_not_promote TARGET "${target}" PROPERTY _qt_no_promote_global)
    set("${out_var}" "${should_not_promote}" PARENT_SCOPE)
endfunction()
