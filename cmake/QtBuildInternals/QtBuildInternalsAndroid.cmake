#
# Android specific functions/macros/properties required for building Qt Modules
#

define_property(TARGET
    PROPERTY
        QT_ANDROID_MODULE_INSTALL_DIR
    BRIEF_DOCS
        "Recorded install location for a Qt Module."
    FULL_DOCS
        "Recorded install location for a Qt Module. Used by qt_android_dependencies()."
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
# Generate Qt Module -android-dependencies.xml required by the
# androiddeploytoolqt to successfully copy all the plugins and other dependent
# items into tha APK
function(qt_android_dependencies target)
    get_target_property(target_type "${target}" TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()

    get_target_property(arg_JAR_DEPENDENCIES ${target} QT_ANDROID_JAR_DEPENDENCIES)
    get_target_property(arg_BUNDLED_JAR_DEPENDENCIES ${target} QT_ANDROID_BUNDLED_JAR_DEPENDENCIES)
    get_target_property(arg_LIB_DEPENDENCIES ${target} QT_ANDROID_LIB_DEPENDENCIES)
    get_target_property(arg_LIB_DEPENDENCY_REPLACEMENTS ${target} QT_ANDROID_LIB_DEPENDENCY_REPLACEMENTS)
    get_target_property(arg_BUNDLED_FILES ${target} QT_ANDROID_BUNDLED_FILES)
    get_target_property(arg_PERMISSIONS ${target} QT_ANDROID_PERMISSIONS)
    get_target_property(module_plugins ${target} MODULE_PLUGIN_TYPES)

    if ((NOT module_plugins)
        AND (NOT arg_JAR_DEPENDENCIES)
        AND (NOT arg_LIB_DEPENDENCY_REPLACEMENTS)
        AND (NOT arg_LIB_DEPENDENCIES)
        AND (NOT arg_BUNDLED_JAR_DEPENDENCIES)
        AND (NOT arg_PERMISSIONS)
        AND (NOT arg_BUNDLED_FILES))
        # None of the values were set, so there's nothing to do
        return()
    endif()


    get_target_property(target_output_name ${target} OUTPUT_NAME)
    if (NOT target_output_name)
        set(target_name ${target})
    else()
        set(target_name ${target_output_name})
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

    set(dependency_file "${QT_BUILD_DIR}/${INSTALL_LIBDIR}/${target_name}_${CMAKE_ANDROID_ARCH_ABI}-android-dependencies.xml")

    set(file_contents "<rules><dependencies>\n")
    string(APPEND file_contents "<lib name=\"${target_name}_${CMAKE_ANDROID_ARCH_ABI}\"><depends>\n")

    # Jar Dependencies
    if(arg_JAR_DEPENDENCIES)
        foreach(jar_dependency IN LISTS arg_JAR_DEPENDENCIES)
            section(${jar_dependency} ":" jar_file init_class)
            if (init_class)
                set(init_class "initClass=\"${init_class}\"")
            endif()
            file(TO_NATIVE_PATH ${jar_file} jar_file_native)
            string(APPEND file_contents "<jar file=\"${jar_file_native}\" ${init_class} />\n")
        endforeach()
    endif()

    # Bundled Jar Dependencies
    if(arg_BUNDLED_JAR_DEPENDENCIES)
        foreach(jar_bundle IN LISTS arg_BUNDLED_JAR_DEPENDENCIES)
            section(${jar_bundle} ":" bundle_file init_calss)
            if (init_class)
                set(init_class "initClass=\"${init_class}\"")
            endif()
            file(TO_NATIVE_PATH ${jar_bundle} jar_bundle_native)
            string(APPEND file_contents "<jar bundling=\"1\" file=\"${jar_bundle_native}\" ${init_class} />\n")
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
            file(TO_NATIVE_PATH ${lib_file} lib_file_native)
            string(APPEND file_contents "<lib file=\"${lib_file_native}\" ${lib_extends} />\n")
        endforeach()
    endif()

    # Lib Dependencies Replacements
    if(arg_LIB_DEPENDENCY_REPLACEMENTS)
        foreach(lib IN LISTS arg_LIB_DEPENDENCY_REPLACEMENTS)
            string(REPLACE ".so" "_${CMAKE_ANDROID_ARCH_ABI}.so" lib ${lib})
            section(${lib} ":" lib_file lib_replacement)
            if (lib_replacement)
                file(TO_NATIVE_PATH ${lib_replacement} lib_replacement_native)
                set(lib_replacement "replaces=\"${lib_replacement_native}\"")
            endif()
            file(TO_NATIVE_PATH ${lib_file} lib_file_native)
            string(APPEND file_contents "<lib file=\"${lib_file_native}\" ${lib_replacement} />\n")
        endforeach()
    endif()


    # Bundled files
    if(arg_BUNDLED_FILES)
        foreach(bundled_file IN LISTS arg_BUNDLED_FILES)
            file(TO_NATIVE_PATH ${bundled_file} file_native)
            string(APPEND file_contents "<bundled file=\"${file_native}\" />\n")
        endforeach()
    endif()

    # Module plugins
    if(module_plugins)
        foreach(plugin IN LISTS module_plugins)
            string(APPEND file_contents "<bundled file=\"plugins/${plugin}\" />\n")
        endforeach()
    endif()

    # Android Permissions
    if(arg_PERMISSIONS)
        foreach(permission IN LISTS arg_PERMISSIONS)
            string(APPEND file_contents "<permission name=\"${permission}\" />\n")
        endforeach()
    endif()

    string(APPEND file_contents "</depends></lib>")
    string(APPEND file_contents "</dependencies></rules>\n")
    file(WRITE ${dependency_file} ${file_contents})

    get_target_property(target_install_dir ${target} QT_ANDROID_MODULE_INSTALL_DIR)
    if (NOT target_install_dir)
        message(SEND_ERROR "qt_android_dependencies: Target ${target} is either not a Qt Module or has no recorded install location")
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


