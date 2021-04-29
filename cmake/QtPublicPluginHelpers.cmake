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

    # Complete condition that defines whether a static plugin is linked
    string(CONCAT _plugin_condition
        "$<BOOL:$<AND:"
            "${_is_plugin_marker_genex},"
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

function(__qt_internal_add_static_plugin_linkage plugin_target qt_module_target)
    __qt_internal_get_static_plugin_condition_genex("${plugin_target}" plugin_condition)

    set(plugin_target "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}")

    # If this condition is true, we link against the plug-in
    set(plugin_genex "$<${plugin_condition}:${plugin_target}>")
    target_link_libraries(${qt_module_target} INTERFACE "${plugin_genex}")
endfunction()

function(__qt_internal_add_static_plugin_import_macro
        plugin_target
        qt_module_target
        qt_module_unprefixed)
    set(_generated_qt_plugin_file_name
        "${CMAKE_CURRENT_BINARY_DIR}/.qt_plugins/qt_${qt_module_unprefixed}_${plugin_target}.cpp")

    set(plugin_target_versioned "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}")
    get_target_property(class_name "${plugin_target_versioned}" QT_PLUGIN_CLASS_NAME)

    set(_generated_qt_plugin_file_content "#include <QtPlugin>\nQ_IMPORT_PLUGIN(${class_name})")

    # Generate a source file to import that plug-in. Be careful not to
    # update the timestamp of the generated file if we are not going to
    # change anything. Otherwise we will trigger CMake's autogen to re-run
    # and executables will then need to at least relink.
    set(need_write TRUE)
    if(EXISTS ${_generated_qt_plugin_file_name})
        file(READ ${_generated_qt_plugin_file_name} old_contents)
        if(old_contents STREQUAL "${_generated_qt_plugin_file_content}")
            set(need_write FALSE)
        endif()
    endif()
    if(need_write)
        file(WRITE "${_generated_qt_plugin_file_name}"
                   "${_generated_qt_plugin_file_content}")
    endif()

    __qt_internal_get_static_plugin_condition_genex("${plugin_target}" plugin_condition)

    target_sources(${qt_module_target} INTERFACE
                   "$<${plugin_condition}:${_generated_qt_plugin_file_name}>")
endfunction()
