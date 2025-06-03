# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#
# Android specific functions/macros/properties required for building Qt Modules
#

macro(qt_internal_setup_android_target_properties)
    define_property(TARGET
        PROPERTY
            QT_ANDROID_MODULE_INSTALL_DIR
        BRIEF_DOCS
            "Recorded install location for a Qt Module."
        FULL_DOCS
            "Recorded install location for a Qt Module. Used by qt_internal_android_dependencies()."
    )

    define_property(TARGET
        PROPERTY
            QT_ANDROID_JAR_DEPENDENCIES
        BRIEF_DOCS
            "Qt Module Jar dependencies list."
        FULL_DOCS
            "Qt Module Jar dependencies list."
    )

    define_property(TARGET
        PROPERTY
            QT_ANDROID_BUNDLED_JAR_DEPENDENCIES
        BRIEF_DOCS
            "Qt Module Jars that should be bundled with it during packing."
        FULL_DOCS
            "Qt Module Jars that should be bundled with it during packing."
    )

    define_property(TARGET
        PROPERTY
            QT_ANDROID_LIB_DEPENDENCIES
        BRIEF_DOCS
            "Qt Module C++ libraries that should be bundled with it during packing."
        FULL_DOCS
            "Qt Module C++ libraries that should be bundled with it during packing."
    )

    define_property(TARGET
        PROPERTY
            QT_ANDROID_LIB_DEPENDENCY_REPLACEMENTS
        BRIEF_DOCS
            "Qt Module C++ libraries that can replace libraries declared with the QT_ANDROID_LIB_DEPENDENCIES property."
        FULL_DOCS
            "Qt Module C++ libraries that can replace libraries declared with the QT_ANDROID_LIB_DEPENDENCIES property."
    )

    define_property(TARGET
        PROPERTY
            QT_ANDROID_BUNDLED_FILES
        BRIEF_DOCS
            "Qt Module files that need to be bundled during packing."
        FULL_DOCS
            "Qt Module files that need to be bundled during packing."
    )

    define_property(TARGET
        PROPERTY
            QT_ANDROID_PERMISSIONS
        BRIEF_DOCS
            "Qt Module android permission list."
        FULL_DOCS
            "Qt Module android permission list."
    )

    define_property(TARGET
        PROPERTY
            QT_ANDROID_FEATURES
        BRIEF_DOCS
            "Qt Module android feature list."
        FULL_DOCS
            "Qt Module android feature list."
    )

    define_property(TARGET
        PROPERTY
            QT_ANDROID_ABIS
        BRIEF_DOCS
            "List of ABIs that the target packages are built with."
        FULL_DOCS
            "List of ABIs that the target packages are built with."
    )
endmacro()

function(qt_internal_android_dependencies_content target file_content_out)
    get_target_property(arg_JAR_DEPENDENCIES ${target} QT_ANDROID_JAR_DEPENDENCIES)
    get_target_property(arg_BUNDLED_JAR_DEPENDENCIES ${target} QT_ANDROID_BUNDLED_JAR_DEPENDENCIES)
    get_target_property(arg_LIB_DEPENDENCIES ${target} QT_ANDROID_LIB_DEPENDENCIES)
    get_target_property(arg_LIB_DEPENDENCY_REPLACEMENTS ${target} QT_ANDROID_LIB_DEPENDENCY_REPLACEMENTS)
    get_target_property(arg_BUNDLED_FILES ${target} QT_ANDROID_BUNDLED_FILES)
    get_target_property(arg_PERMISSIONS ${target} QT_ANDROID_PERMISSIONS)
    get_target_property(arg_FEATURES ${target} QT_ANDROID_FEATURES)

    if ((NOT arg_JAR_DEPENDENCIES)
        AND (NOT arg_BUNDLED_JAR_DEPENDENCIES)
        AND (NOT arg_LIB_DEPENDENCIES)
        AND (NOT arg_LIB_DEPENDENCY_REPLACEMENTS)
        AND (NOT arg_BUNDLED_FILES)
        AND (NOT arg_PERMISSIONS)
        AND (NOT arg_FEATURES))
        # None of the values were set, so there's nothing to do
        return()
    endif()

    # mimic qmake's section and string splitting from
    # mkspecs/feature/qt_android_deps.prf
    macro(section string delimiter first second)
        string(FIND ${string} ${delimiter} delimiter_location)
        if (NOT ${delimiter_location} EQUAL -1)
            string(SUBSTRING ${string} 0 ${delimiter_location} ${first})
            math(EXPR delimiter_location "${delimiter_location} + 1")
            string(SUBSTRING ${string} ${delimiter_location} -1 ${second})
        else()
            set(${first} ${string})
            set(${second} "")
        endif()
    endmacro()

    set(file_contents "")

    # Jar Dependencies
    if(arg_JAR_DEPENDENCIES)
        foreach(jar_dependency IN LISTS arg_JAR_DEPENDENCIES)
            section(${jar_dependency} ":" jar_file init_class)
            if (init_class)
                set(init_class "initClass=\"${init_class}\"")
            endif()
            # Use unix path to allow using files on any host platform.
            file(TO_CMAKE_PATH ${jar_file} jar_file_unix_path)
            string(APPEND file_contents "<jar file=\"${jar_file_unix_path}\" ${init_class} />\n")
        endforeach()
    endif()

    # Bundled Jar Dependencies
    if(arg_BUNDLED_JAR_DEPENDENCIES)
        foreach(jar_bundle IN LISTS arg_BUNDLED_JAR_DEPENDENCIES)
            section(${jar_bundle} ":" bundle_file init_class)
            if (init_class)
                set(init_class "initClass=\"${init_class}\"")
            endif()
            # Use unix path to allow using files on any host platform.
            file(TO_CMAKE_PATH ${bundle_file} jar_bundle_unix_path)
            string(APPEND file_contents
                   "<jar bundling=\"1\" file=\"${jar_bundle_unix_path}\" ${init_class} />\n")
        endforeach()
    endif()

    # Lib Dependencies
    if(arg_LIB_DEPENDENCIES)
        foreach(lib IN LISTS arg_LIB_DEPENDENCIES)
            string(REPLACE ".so" "_${CMAKE_ANDROID_ARCH_ABI}.so" lib ${lib})
            section(${lib} ":" lib_file lib_extends)
            if (lib_extends)
                set(lib_extends "extends=\"${lib_extends}\"")
            endif()
            # Use unix path to allow using files on any host platform.
            file(TO_CMAKE_PATH ${lib_file} lib_file_unix_path)
            string(APPEND file_contents "<lib file=\"${lib_file_unix_path}\" ${lib_extends} />\n")
        endforeach()
    endif()

    # Lib Dependencies Replacements
    if(arg_LIB_DEPENDENCY_REPLACEMENTS)
        foreach(lib IN LISTS arg_LIB_DEPENDENCY_REPLACEMENTS)
            string(REPLACE ".so" "_${CMAKE_ANDROID_ARCH_ABI}.so" lib ${lib})
            section(${lib} ":" lib_file lib_replacement)
            if (lib_replacement)
                # Use unix path to allow using files on any host platform.
                file(TO_CMAKE_PATH ${lib_replacement} lib_replacement_unix_path)
                set(lib_replacement "replaces=\"${lib_replacement_unix_path}\"")
            endif()
            # Use unix path to allow using files on any host platform.
            file(TO_CMAKE_PATH ${lib_file} lib_file_unix_path)
            string(APPEND file_contents
                   "<lib file=\"${lib_file_unix_path}\" ${lib_replacement} />\n")
        endforeach()
    endif()

    # Bundled files
    if(arg_BUNDLED_FILES)
        foreach(bundled_file IN LISTS arg_BUNDLED_FILES)
            # Use unix path to allow using files on any host platform.
            file(TO_CMAKE_PATH ${bundled_file} file_unix_path)
            string(APPEND file_contents "<bundled file=\"${file_unix_path}\" />\n")
        endforeach()
    endif()

    # Android Permissions
    if(arg_PERMISSIONS)
        foreach(permission IN LISTS arg_PERMISSIONS)
            string(APPEND file_contents "<permission name=\"${permission}\" />\n")
        endforeach()
    endif()

    # Android Features
    if(arg_FEATURES)
        foreach(feature IN LISTS arg_FEATURES)
            string(APPEND file_contents "<feature name=\"${feature}\" />\n")
        endforeach()
    endif()

    set(${file_content_out} ${file_contents} PARENT_SCOPE)
endfunction()

# Generate Qt Module -android-dependencies.xml required by the
# androiddeploytoolqt to successfully copy all the plugins and other dependent
# items into the APK
function(qt_internal_android_dependencies target)
    get_target_property(target_type "${target}" TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()

    # Get plugins for the current module
    get_target_property(module_plugin_types ${target} MODULE_PLUGIN_TYPES)

    # Get depends for the current module
    qt_internal_android_dependencies_content(${target} file_contents)

    # Get plugins from the module's plugin types and get their dependencies
    foreach(plugin ${QT_KNOWN_PLUGINS})
        get_target_property(iter_known_plugin_type ${plugin} QT_PLUGIN_TYPE)
        foreach(plugin_type ${module_plugin_types})
            if (plugin_type STREQUAL iter_known_plugin_type)
                qt_internal_android_dependencies_content(${plugin} plugin_file_contents)
                string(APPEND file_contents ${plugin_file_contents})
            endif()
        endforeach()
    endforeach()

    if ((NOT module_plugin_types)
        AND (NOT file_contents))
        # None of the values were set, so there's nothing to do
        return()
    endif()

    get_target_property(target_output_name ${target} OUTPUT_NAME)
    if (NOT target_output_name)
        set(target_name ${target})
    else()
        set(target_name ${target_output_name})
    endif()

    string(PREPEND file_contents "<lib name=\"${target_name}_${CMAKE_ANDROID_ARCH_ABI}\"><depends>\n")
    string(PREPEND file_contents "<rules><dependencies>\n")

    # Module plugins
    if(module_plugin_types)
        foreach(plugin IN LISTS module_plugin_types)
            string(APPEND file_contents
                "<bundled file=\"${INSTALL_PLUGINSDIR}/${plugin}\" type=\"plugin_dir\"/>\n")
        endforeach()
    endif()

    string(APPEND file_contents "</depends></lib>\n")
    string(APPEND file_contents "</dependencies></rules>")

    qt_path_join(dependency_file "${QT_BUILD_DIR}" "${INSTALL_LIBDIR}" "${target_name}_${CMAKE_ANDROID_ARCH_ABI}-android-dependencies.xml")
    file(WRITE ${dependency_file} ${file_contents})

    get_target_property(target_install_dir ${target} QT_ANDROID_MODULE_INSTALL_DIR)
    if (NOT target_install_dir)
        message(SEND_ERROR "qt_internal_android_dependencies: Target ${target} is either not a Qt Module or has no recorded install location")
        return()
    endif()

    # Copy file into install directory, required by the androiddeployqt tool.
    qt_install(FILES
        ${dependency_file}
        DESTINATION
            ${target_install_dir}
        COMPONENT
            Devel)
endfunction()
