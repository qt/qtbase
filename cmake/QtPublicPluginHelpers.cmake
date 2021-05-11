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

    # The code in here uses the properties defined in qt_import_plugins (Qt6CoreMacros.cmake)
    set(_plugin_is_default "$<TARGET_PROPERTY:${plugin_target},QT_DEFAULT_PLUGIN>")

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

# Link plugin via usage requirements of associated Qt module.
function(__qt_internal_add_static_plugin_linkage plugin_target qt_module_target)
    __qt_internal_get_static_plugin_condition_genex("${plugin_target}" plugin_condition)

    set(plugin_target "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}")

    # If this condition is true, we link against the plug-in
    set(plugin_genex "$<${plugin_condition}:${plugin_target}>")
    target_link_libraries(${qt_module_target} INTERFACE "${plugin_genex}")
endfunction()

# Generates C++ import macro source code for given plugin
function(__qt_internal_get_plugin_import_macro plugin_target out_var)
    set(plugin_target_versioned "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}")
    get_target_property(class_name "${plugin_target_versioned}" QT_PLUGIN_CLASS_NAME)
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
    set(objs_genex "$<TARGET_OBJECTS:${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_init_target}>")

    target_sources(${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target} INTERFACE
                   "${objs_genex}")
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

    __qt_internal_get_static_plugin_init_target_name("${plugin_target}" plugin_init_target)

    add_library("${plugin_init_target}" OBJECT "${generated_qt_plugin_file_name}")
    target_link_libraries(${plugin_init_target}
        PRIVATE

        # Core provides the symbols needed by Q_IMPORT_PLUGIN.
        ${QT_CMAKE_EXPORT_NAMESPACE}::Core
    )

    set(${out_var_plugin_init_target} "${plugin_init_target}" PARENT_SCOPE)
endfunction()

