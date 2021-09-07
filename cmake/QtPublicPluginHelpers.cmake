# Gets the qt plugin type of the given plugin into out_var_plugin_type.
# Also sets out_var_has_plugin_type to TRUE or FALSE depending on whether the plugin type was found.
function(__qt_internal_plugin_get_plugin_type
         plugin_target
         out_var_has_plugin_type
         out_var_plugin_type)
    set(has_plugin_type TRUE)

    set(plugin_target_versioned "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}")

    get_target_property(_plugin_type "${plugin_target_versioned}" QT_PLUGIN_TYPE)
    if(NOT _plugin_type)
        message("Warning: plugin ${plugin_target_versioned} has no plugin type set, skipping.")
        set(has_plugin_type FALSE)
    else()
        set(${out_var_plugin_type} "${_plugin_type}" PARENT_SCOPE)
    endif()

    set(${out_var_has_plugin_type} "${has_plugin_type}" PARENT_SCOPE)
endfunction()

# Gets the qt plugin class name of the given target into out_var.
function(__qt_internal_plugin_has_class_name plugin_target out_var)
    set(plugin_target_versioned "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}")

    get_target_property(classname "${plugin_target_versioned}" QT_PLUGIN_CLASS_NAME)
    if(NOT classname)
        message("Warning: plugin ${plugin_target_versioned} has no class name, skipping.")
    endif()

    # If unset, it will be -NOTFOUND and still evaluate to false.
    set(${out_var} "${classname}" PARENT_SCOPE)
endfunction()

# Constructs a generator expression which decides whether a plugin will be used.
#
# The conditions are based on the various properties set in qt_import_plugins.

# All the TARGET_PROPERTY genexes are evaluated in the context  of the currently linked target.
#
# The genex is saved into out_var.
function(__qt_internal_get_static_plugin_condition_genex
         plugin_target_unprefixed
         out_var)

    set(plugin_target "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target_unprefixed}")
    set(plugin_target_versionless "Qt::${plugin_target_unprefixed}")

    get_target_property(_plugin_type "${plugin_target}" QT_PLUGIN_TYPE)

    set(_default_plugins_are_enabled
        "$<NOT:$<STREQUAL:$<GENEX_EVAL:$<TARGET_PROPERTY:QT_DEFAULT_PLUGINS>>,0>>")
    set(_manual_plugins_genex "$<GENEX_EVAL:$<TARGET_PROPERTY:QT_PLUGINS>>")
    set(_no_plugins_genex "$<GENEX_EVAL:$<TARGET_PROPERTY:QT_NO_PLUGINS>>")

    # Plugin genex marker for prl processing.
    set(_is_plugin_marker_genex "$<BOOL:QT_IS_PLUGIN_GENEX>")

    set(_plugin_is_default "$<TARGET_PROPERTY:${plugin_target},QT_DEFAULT_PLUGIN>")

    # The code in here uses the properties defined in qt_import_plugins (Qt6CoreMacros.cmake)

    # INCLUDE
    set(_plugin_is_whitelisted "$<IN_LIST:${plugin_target},${_manual_plugins_genex}>")
    set(_plugin_versionless_is_whitelisted
        "$<IN_LIST:${plugin_target_versionless},${_manual_plugins_genex}>")

    # Note: qt_import_plugins sets the QT_PLUGINS_${_plugin_type} to "-"
    # when excluding it with EXCLUDE_BY_TYPE,
    # which ensures that no plug-in will be supported unless explicitly re-added afterwards.
    string(CONCAT _plugin_is_not_blacklisted
        "$<AND:"
            "$<NOT:" # EXCLUDE
                "$<IN_LIST:${plugin_target},${_no_plugins_genex}>"
            ">,"
            "$<NOT:"
                "$<IN_LIST:${plugin_target_versionless},${_no_plugins_genex}>"
            ">,"
            # Excludes both plugins targeted by EXCLUDE_BY_TYPE and not included in
            # INCLUDE_BY_TYPE.
            "$<STREQUAL:,$<GENEX_EVAL:$<TARGET_PROPERTY:QT_PLUGINS_${_plugin_type}>>>"
        ">"
    )

    # Support INCLUDE_BY_TYPE
    string(CONCAT _plugin_is_in_type_whitelist
        "$<IN_LIST:"
            "${plugin_target},"
            "$<GENEX_EVAL:"
                "$<TARGET_PROPERTY:QT_PLUGINS_${_plugin_type}>"
            ">"
        ">"
    )
    string(CONCAT _plugin_versionless_is_in_type_whitelist
        "$<IN_LIST:"
            "${plugin_target_versionless},"
            "$<GENEX_EVAL:"
                "$<TARGET_PROPERTY:QT_PLUGINS_${_plugin_type}>"
            ">"
        ">"
    )

    # No point in linking the plugin initialization source file into static libraries. The
    # initialization symbol will be discarded by the linker when the static lib is linked into an
    # executable or shared library, because nothing is referencing the global static symbol.
    set(type_genex "$<TARGET_PROPERTY:TYPE>")
    set(no_static_genex "$<NOT:$<STREQUAL:${type_genex},STATIC_LIBRARY>>")

    # Complete condition that defines whether a static plugin is linked
    string(CONCAT _plugin_condition
        "$<BOOL:$<AND:"
            "${_is_plugin_marker_genex},"
            "${no_static_genex},"
            "$<OR:"
                "${_plugin_is_whitelisted},"
                "${_plugin_versionless_is_whitelisted},"
                "${_plugin_is_in_type_whitelist},"
                "${_plugin_versionless_is_in_type_whitelist},"
                "$<AND:"
                    "${_default_plugins_are_enabled},"
                    "${_plugin_is_default},"
                    "${_plugin_is_not_blacklisted}"
                ">"
            ">"
        ">>"
    )

    set(${out_var} "${_plugin_condition}" PARENT_SCOPE)
endfunction()

# Wraps the genex condition to evaluate to true only when using the regular plugin importing mode
# (not finalizer mode).
function(__qt_internal_get_plugin_condition_regular_mode plugin_condition out_var)
    set(not_finalizer_mode "$<NOT:$<BOOL:$<TARGET_PROPERTY:_qt_static_plugins_finalizer_mode>>>")
    set(full_plugin_condition "$<AND:${plugin_condition},${not_finalizer_mode}>")
    set(${out_var} "${full_plugin_condition}" PARENT_SCOPE)
endfunction()

# Link plugin via usage requirements of associated Qt module.
function(__qt_internal_add_static_plugin_linkage plugin_target qt_module_target)
    __qt_internal_get_static_plugin_condition_genex("${plugin_target}" plugin_condition)
    __qt_internal_get_plugin_condition_regular_mode("${plugin_condition}" full_plugin_condition)

    set(plugin_target "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}")

    # If this condition is true, we link against the plug-in
    set(plugin_genex "$<${full_plugin_condition}:${plugin_target}>")
    target_link_libraries(${qt_module_target} INTERFACE "${plugin_genex}")
endfunction()

# Generates C++ import macro source code for given plugin
function(__qt_internal_get_plugin_import_macro plugin_target out_var)
    set(plugin_target_prefixed "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}")

    # Query the class name of plugin targets prefixed with a Qt namespace and without, this is
    # needed to support plugin object initializers created by user projects.
    set(class_name"")
    set(class_name_prefixed "")

    if(TARGET ${plugin_target})
        get_target_property(class_name "${plugin_target}" QT_PLUGIN_CLASS_NAME)
    endif()

    if(TARGET ${plugin_target_prefixed})
        get_target_property(class_name_prefixed "${plugin_target_prefixed}" QT_PLUGIN_CLASS_NAME)
    endif()

    if(NOT class_name AND NOT class_name_prefixed)
        message(FATAL_ERROR "No QT_PLUGIN_CLASS_NAME value on target: '${plugin_target}'")
    endif()

    # Qt prefixed target takes priority.
    if(class_name_prefixed)
        set(class_name "${class_name_prefixed}")
    endif()

    set(${out_var} "Q_IMPORT_PLUGIN(${class_name})" PARENT_SCOPE)
endfunction()

function(__qt_internal_get_plugin_include_prelude out_var)
    set(${out_var} "#include <QtPlugin>\n" PARENT_SCOPE)
endfunction()

# Set up plugin initialization via usage requirements of associated Qt module.
#
# Adds the plugin init object library as an INTERFACE source of the plugin target.
# This is similar to how it was done before, except instead of generating a C++ file and compiling
# it as part of the user project, we just specify the pre-compiled object file as an INTERFACE
# source so that user projects don't have to compile it. User project builds will thus be shorter.
function(__qt_internal_add_static_plugin_import_macro
        plugin_target
        qt_module_target
        qt_module_unprefixed)

    __qt_internal_get_static_plugin_init_target_name("${plugin_target}" plugin_init_target)
    set(plugin_init_target_namespaced "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_init_target}")

    __qt_internal_propagate_object_library(
        "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}"
        "${plugin_init_target_namespaced}"
        EXTRA_CONDITIONS
            "$<NOT:$<BOOL:$<TARGET_PROPERTY:_qt_static_plugins_finalizer_mode>>>"
    )
endfunction()

# Get target name of object library which is used to initialize a qt plugin.
function(__qt_internal_get_static_plugin_init_target_name plugin_target out_var)
    # Keep the target name short, so we don't hit long path issues on Windows.
    set(plugin_init_target "${plugin_target}_init")

    set(${out_var} "${plugin_init_target}" PARENT_SCOPE)
endfunction()

# Create an object library that initializes a static qt plugin.
#
# The object library contains a single generated C++ file that calls Q_IMPORT_PLUGIN(plugin_class).
# The object library is exported as part of the Qt build and consumed by user applications
# that link to qt plugins.
#
# The created target name is assigned to 'out_var_plugin_init_target'.
function(__qt_internal_add_static_plugin_init_object_library
        plugin_target
        out_var_plugin_init_target)

    __qt_internal_get_plugin_import_macro(${plugin_target} import_macro)
    __qt_internal_get_plugin_include_prelude(include_prelude)
    set(import_content "${include_prelude}${import_macro}")

    string(MAKE_C_IDENTIFIER "${plugin_target}" plugin_target_identifier)
    set(generated_qt_plugin_file_name
        "${CMAKE_CURRENT_BINARY_DIR}/${plugin_target_identifier}_init.cpp")

    file(GENERATE
        OUTPUT "${generated_qt_plugin_file_name}"
        CONTENT "${import_content}"
    )

    # CMake versions earlier than 3.18.0 can't find the generated file for some reason,
    # failing at generation phase.
    # Explicitly marking the file as GENERATED fixes the issue.
    set_source_files_properties("${generated_qt_plugin_file_name}" PROPERTIES GENERATED TRUE)

    __qt_internal_get_static_plugin_init_target_name("${plugin_target}" plugin_init_target)

    qt6_add_library("${plugin_init_target}" OBJECT "${generated_qt_plugin_file_name}")
    target_link_libraries(${plugin_init_target}
        PRIVATE

        # Core provides the symbols needed by Q_IMPORT_PLUGIN.
        ${QT_CMAKE_EXPORT_NAMESPACE}::Core
    )

    set_property(TARGET ${plugin_init_target} PROPERTY _is_qt_plugin_init_target TRUE)
    set_property(TARGET ${plugin_init_target} APPEND PROPERTY
        EXPORT_PROPERTIES _is_qt_plugin_init_target
    )

    set(${out_var_plugin_init_target} "${plugin_init_target}" PARENT_SCOPE)
endfunction()

# Collect a list of genexes to link plugin libraries.
function(__qt_internal_collect_plugin_libraries plugin_targets out_var)
    set(plugins_to_link "")

    foreach(plugin_target ${plugin_targets})
        __qt_internal_get_static_plugin_condition_genex(
            "${plugin_target}"
            plugin_condition)

        set(plugin_target_versioned "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}")

        list(APPEND plugins_to_link "$<${plugin_condition}:${plugin_target_versioned}>")
    endforeach()

    set("${out_var}" "${plugins_to_link}" PARENT_SCOPE)
endfunction()

# Collect a list of genexes to link plugin initializer object libraries.
#
# The object libraries are only linked if the associated plugins are linked.
function(__qt_internal_collect_plugin_init_libraries plugin_targets out_var)
    set(plugin_inits_to_link "")

    foreach(plugin_target ${plugin_targets})
        __qt_internal_get_static_plugin_condition_genex(
            "${plugin_target}"
            plugin_condition)

        __qt_internal_get_static_plugin_init_target_name("${plugin_target}" plugin_init_target)
        set(plugin_init_target_versioned "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_init_target}")

        list(APPEND plugin_inits_to_link "$<${plugin_condition}:${plugin_init_target_versioned}>")
    endforeach()

    set("${out_var}" "${plugin_inits_to_link}" PARENT_SCOPE)
endfunction()

# Collects all plugin targets discovered by walking the dependencies of ${target}.
#
# Walks immediate dependencies and their transitive dependencies.
# Plugins are collected by inspecting the _qt_plugins property found on any dependency Qt target.
function(__qt_internal_collect_plugin_targets_from_dependencies target out_var)
    set(dep_targets "")

    __qt_internal_collect_all_target_dependencies("${target}" dep_targets)

    set(plugin_targets "")
    foreach(dep_target ${dep_targets})
        get_target_property(plugins ${dep_target} _qt_plugins)
        if(plugins)
            list(APPEND plugin_targets ${plugins})
        endif()
    endforeach()

    # Plugins that are specified via qt_import_plugin's INCLUDE or INCLUDE_BY_TYPE can have
    # dependencies on Qt modules. These modules in turn might bring in more default plugins to link
    # So it's recursive. Do only one pass for now. Try to extract the included static plugins, walk
    # their public and private dependencies, check if any of them are Qt modules that provide more
    # plugins and extract the target names of those plugins.
    __qt_internal_collect_plugin_targets_from_dependencies_of_plugins(
        "${target}" recursive_plugin_targets)
    if(recursive_plugin_targets)
        list(APPEND plugin_targets ${recursive_plugin_targets})
    endif()
    list(REMOVE_DUPLICATES plugin_targets)

    set("${out_var}" "${plugin_targets}" PARENT_SCOPE)
endfunction()

# Helper to collect plugin targets from encountered module dependencies as a result of walking
# dependencies. These module dependencies might expose additional plugins.
function(__qt_internal_collect_plugin_targets_from_dependencies_of_plugins target out_var)
    set(assigned_plugins_overall "")

    get_target_property(assigned_qt_plugins "${target}" QT_PLUGINS)

    if(assigned_qt_plugins)
        foreach(assigned_qt_plugin ${assigned_qt_plugins})
            if(TARGET "${assigned_qt_plugin}")
                list(APPEND assigned_plugins_overall "${assigned_qt_plugin}")
            endif()
        endforeach()
    endif()

    get_target_property(assigned_qt_plugins_by_type "${target}" _qt_plugins_by_type)

    if(assigned_qt_plugins_by_type)
        foreach(assigned_qt_plugin ${assigned_qt_plugins_by_type})
            if(TARGET "${assigned_qt_plugin}")
                list(APPEND assigned_plugins_overall "${assigned_qt_plugin}")
            endif()
        endforeach()
    endif()

    set(plugin_targets "")
    foreach(target ${assigned_plugins_overall})
        __qt_internal_walk_libs(
            "${target}"
            dep_targets
            _discarded_out_var
            "qt_private_link_library_targets"
            "collect_targets")

        foreach(dep_target ${dep_targets})
            get_target_property(plugins ${dep_target} _qt_plugins)
            if(plugins)
                list(APPEND plugin_targets ${plugins})
            endif()
        endforeach()
    endforeach()

    list(REMOVE_DUPLICATES plugin_targets)

    set("${out_var}" "${plugin_targets}" PARENT_SCOPE)
endfunction()

# Main logic of finalizer mode.
function(__qt_internal_apply_plugin_imports_finalizer_mode target)
    # Nothing to do in a shared build.
    if(QT6_IS_SHARED_LIBS_BUILD)
        return()
    endif()

    # Process a target only once.
    get_target_property(processed ${target} _qt_plugin_finalizer_imports_processed)
    if(processed)
        return()
    endif()

    # By default if the project hasn't explicitly opted in or out, use finalizer mode.
    # The precondition for this is that qt_finalize_target was called (either explicitly by the user
    # or auto-deferred by CMake 3.19+).
    __qt_internal_check_finalizer_mode("${target}"
        use_finalizer_mode
        static_plugins
        DEFAULT_VALUE "TRUE"
    )

    if(NOT use_finalizer_mode)
        return()
    endif()

    __qt_internal_collect_plugin_targets_from_dependencies("${target}" plugin_targets)
    __qt_internal_collect_plugin_init_libraries("${plugin_targets}" init_libraries)
    __qt_internal_collect_plugin_libraries("${plugin_targets}" plugin_libraries)

    target_link_libraries(${target} PRIVATE "${plugin_libraries}" "${init_libraries}")

    set_target_properties(${target} PROPERTIES _qt_plugin_finalizer_imports_processed TRUE)
endfunction()
