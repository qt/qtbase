include(CMakePackageConfigHelpers)

# Install locations:
set(INSTALL_BINDIR "bin" CACHE PATH "Executables [PREFIX/bin]")
set(INSTALL_INCLUDEDIR "include" CACHE PATH "Header files [PREFIX/include]")
set(INSTALL_LIBDIR "lib" CACHE PATH "Libraries [PREFIX/lib]")
set(INSTALL_ARCHDATADIR "." CACHE PATH "Arch-dependent data [PREFIX]")
set(INSTALL_PLUGINSDIR "${INSTALL_ARCHDATADIR}/plugins" CACHE PATH
    "Plugins [ARCHDATADIR/plugins]")

set(INSTALL_TARGETS_DEFAULT_ARGS
    RUNTIME DESTINATION "${INSTALL_BINDIR}"
    LIBRARY DESTINATION "${INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${INSTALL_LIBDIR}" COMPONENT Devel
    INCLUDES DESTINATION "${INSTALL_INCLUDEDIR}"
)

if (WIN32)
    set(_default_libexec "${INSTALL_ARCHDATADIR}/bin")
else()
    set(_default_libexec "${INSTALL_ARCHDATADIR}/libexec")
endif()

set(INSTALL_LIBEXECDIR "${_default_libexec}" CACHE PATH
    "Helper programs [ARCHDATADIR/bin on Windows, ARCHDATADIR/libexec otherwise]")
set(INSTALL_IMPORTDIR "${INSTALL_ARCHDATADIR}/imports" CACHE PATH
    "QML1 imports [ARCHDATADIR/imports]")
set(INSTALL_QMLDIR "${INSTALL_ARCHDATADIR}/qml" CACHE PATH
    "QML2 imports [ARCHDATADIR/qml]")
set(INSTALL_DATADIR "." CACHE PATH  "Arch-independent data [PREFIX]")
set(INSTALL_DOCDIR "${INSTALL_DATADIR}/doc" CACHE PATH "Documentation [DATADIR/doc]")
set(INSTALL_TRANSLATIONSDIR "${INSTALL_DATADIR}/translations" CACHE PATH
    "Translations [DATADIR/translations]")
set(INSTALL_SYSCONFDIR "etc/xdg" CACHE PATH
    "Settings used by Qt programs [PREFIX/etc/xdg]")
set(INSTALL_EXAMPLESDIR "examples" CACHE PATH "Examples [PREFIX/examples]")
set(INSTALL_TESTSDIR "tests" CACHE PATH "Tests [PREFIX/tests]")

# Platform define path, etc.
if(WIN32)
    set(QT_DEFAULT_PLATFORM_DEFINITIONS UNICODE _UNICODE WIN32 _ENABLE_EXTENDED_ALIGNED_STORAGE)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        list(APPEND QT_DEFAULT_PLATFORM_DEFINITIONS WIN64 _WIN64)
    endif()
    if(MSVC)
        set(QT_DEFAULT_PLATFORM_DEFINITION_DIR mkspecs/win32-msvc)
    elseif(CLANG)
        set(QT_DEFAULT_PLATFORM_DEFINITION_DIR mkspecs/win32-clang)
    endif()
elseif(LINUX)
    if(GCC)
        set(QT_DEFAULT_PLATFORM_DEFINITION_DIR mkspecs/linux-g++)
    elseif(CLANG)
        set(QT_DEFAULT_PLATFORM_DEFINITION_DIR mkspecs/linux-clang)
    endif()
elseif(APPLE)
    set(QT_DEFAULT_PLATFORM_DEFINITION_DIR mkspecs/macx-clang)
endif()

if(NOT DEFINED QT_DEFAULT_PLATFORM_DEFINITIONS)
    set(QT_DEFAULT_PLATFORM_DEFINITIONS "")
endif()

set(QT_PLATFORM_DEFINITIONS ${QT_DEFAULT_PLATFORM_DEFINITIONS}
    CACHE STRING "Qt platform specific pre-processor defines")
set(QT_PLATFORM_DEFINITION_DIR ${QT_DEFAULT_PLATFORM_DEFINITION_DIR}
    CACHE PATH "Path to directory that contains qplatformdefs.h")
set(QT_NAMESPACE "" CACHE STRING "Qt Namespace")

macro(_set_known_qt_modules)
    set(KNOWN_QT_MODULES ${ARGN} CACHE INTERNAL "Known Qt modules" FORCE)
endmacro()

# Reset:
_set_known_qt_modules("")

# For adjusting variables when running tests, we need to know what
# the correct variable is for separating entries in PATH-alike
# variables.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(QT_PATH_SEPARATOR "\\;")
else()
    set(QT_PATH_SEPARATOR ":")
endif()


# Functions and macros:

# Print all variables defined in the current scope.
macro(qt_debug_print_variables)
    cmake_parse_arguments(__arg "DEDUP" "" "MATCH;IGNORE" ${ARGN})
    message("Known Variables:")
    get_cmake_property(__variableNames VARIABLES)
    list (SORT __variableNames)
    if (__arg_DEDUP)
        list(REMOVE_DUPLICATES __variableNames)
    endif()

    foreach(__var ${__variableNames})
        set(__ignore OFF)
        foreach(__i ${__arg_IGNORE})
            if(__var MATCHES "${__i}")
                set(__ignore ON)
                break()
            endif()
        endforeach()

        if (__ignore)
            continue()
        endif()

        set(__show OFF)
        foreach(__i ${__arg_MATCH})
            if(__var MATCHES "${__i}")
                set(__show ON)
                break()
            endif()
        endforeach()

        if (__show)
            message("    ${__var}=${${__var}}.")
        endif()
    endforeach()
endmacro()


macro(assert)
    if (${ARGN})
    else()
        message(FATAL_ERROR "ASSERT: ${ARGN}.")
    endif()
endmacro()


function(qt_ensure_perl)
    if(DEFINED HOST_PERL)
        return()
    endif()
    find_program(HOST_PERL "perl" DOC "Perl binary")
    if (NOT HOST_PERL)
        message(FATAL_ERROR "Perl needs to be available to build Qt.")
    endif()
endfunction()


# A version of cmake_parse_arguments that makes sure all arguments are processed and errors out
# with a message about ${type} having received unknown arguments.
macro(qt_parse_all_arguments result type flags options multiopts)
    cmake_parse_arguments(${result} "${flags}" "${options}" "${multiopts}" ${ARGN})
    if(DEFINED ${result}_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments were passed to ${type} (${${result}_UNPARSED_ARGUMENTS}).")
    endif()
endmacro()


# Determines the directory where the generated class-style header files for
# the specified module are located and stores the result in the given result
# variable.
function(qt_internal_module_include_dir result module)
    set(${result} "${PROJECT_BINARY_DIR}/include/${module}" PARENT_SCOPE)
endfunction()

function(qt_internal_add_link_flags target to_add)
    get_target_property(flags "${target}" LINK_FLAGS)
    if ("${flags}" STREQUAL "flags-NOTFOUND")
        set(flags "")
    endif()
    string(APPEND flags " ${to_add}")
    set_target_properties("${target}" PROPERTIES LINK_FLAGS "${flags}")
endfunction()

function(qt_internal_add_linker_version_script target)
    qt_parse_all_arguments(arg "qt_internal_add_linker" "INTERNAL" "" "PRIVATE_HEADERS" ${ARGN})

    if (TEST_ld_version_script)
        if (arg_INTERNAL)
            set(contents "Qt_${PROJECT_VERSION_MAJOR}_PRIVATE_API { *; };")
        else()
            set(contents "Qt_${PROJECT_VERSION_MAJOR}_PRIVATE_API {\n    qt_private_api_tag*;\n")
            foreach(ph ${arg_PRIVATE_HEADERS})
                string(APPEND contents "    @FILE:${ph}@\n")
            endforeach()
            string(APPEND contents "};\n")
            set(current "Qt_${PROJECT_VERSION_MAJOR}")
            if (QT_NAMESPACE STREQUAL "")
                set(tag_symbol "qt_version_tag")
            else()
                set(tag_symbol "qt_version_tag_${QT_NAMESPACE}")
            endif()
            string(APPEND contents "${current} { *; };\n")

            foreach(minor_version RANGE ${PROJECT_VERSION_MINOR})
                set(previous "${current}")
                set(current "Qt_${PROJECT_VERSION_MAJOR}.${minor_version}")
                if (minor_version EQUAL ${PROJECT_VERSION_MINOR})
                   string(APPEND contents "${current} { ${tag_symbol}; } ${previous};\n")
                else()
                   string(APPEND contents "${current} {} ${previous};\n")
                endif()
            endforeach()

            set(infile "${CMAKE_CURRENT_BINARY_DIR}/${target}.version.in")
            set(outfile "${CMAKE_CURRENT_BINARY_DIR}/${target}.version")

            file(GENERATE OUTPUT "${infile}" CONTENT "${contents}")

            qt_ensure_perl()

            add_custom_command(TARGET "${target}" PRE_LINK
                COMMAND "${HOST_PERL}" "${PROJECT_SOURCE_DIR}/mkspecs/features/data/unix/findclasslist.pl" < "${infile}" > "${outfile}"
                BYPRODUCTS "${outfile}" DEPENDS "${infile}"
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                COMMENT "Generating version linker script"
            )
            qt_internal_add_link_flags("${target}" "-Wl,--version-script,${outfile}")
        endif()
    endif()
endfunction()

# Generates the necessary rules to run moc on the sources specified after the MOC parameter.
# The resulting moc files are added to the target.
function(qt_internal_wrap_cpp target)
    # get include dirs
    qt_get_moc_flags(moc_flags)
    qt_parse_all_arguments(arg "qt_internal_wrap_cpp" "" "HEADER_FILE_ONLY" "OPTIONS;DEPENDS;MOC" ${ARGN})

    set(outfiles)

    get_target_property(binary_dir "${target}" BINARY_DIR)
    get_target_property(source_dir "${target}" SOURCE_DIR)

    foreach(it ${arg_MOC})
        get_filename_component(moc_file_extension ${it} EXT)

        if("x${moc_file_extension}" STREQUAL "x.h")
            set(file_prefix "moc_")
            set(file_extension ".cpp")
        else()
            set(file_prefix "")
            set(file_extension ".moc")
        endif()

        qt_make_output_file("${it}" "${file_prefix}" "${file_extension}" "${source_dir}" "${binary_dir}" outfile)

        qt_create_moc_command("${target}" "${source_dir}" "${it}" "${outfile}" "${moc_flags}" "${arg_OPTIONS}" "${arg_DEPENDS}")
        set_source_files_properties("${outfile}" PROPERTIES HEADER_FILE_ONLY ${arg_HEADER_FILE_ONLY})
        if (arg_HEADER_FILE_ONLY)
            get_filename_component(directory "${outfile}" DIRECTORY)
            target_include_directories("${target}" PRIVATE "${directory}")
        endif()

        list(APPEND outfiles "${outfile}")
    endforeach()
    target_sources("${target}" PRIVATE "${outfiles}")
endfunction()


# Get a set of Qt module related values based on the target name.
# When doing qt_internal_module_info(foo Core) this method will set
# the following variables in the caller's scope:
#  * foo with the value "QtCore"
#  * foo_versioned with the value "Qt5Core" (based on major Qt version)
#  * foo_upper with the value "CORE"
#  * foo_lower with the value "core"
function(qt_internal_module_info result target)
    set("${result}" "Qt${target}" PARENT_SCOPE)
    set("${result}_versioned" "Qt${PROJECT_VERSION_MAJOR}${target}" PARENT_SCOPE)
    string(TOUPPER "${target}" upper)
    string(TOLOWER "${target}" lower)
    set("${result}_upper" "${upper}" PARENT_SCOPE)
    set("${result}_lower" "${lower}" PARENT_SCOPE)
endfunction()


# This function takes a target as a parameter, followed by a list of sources.
# The sources are scanned for Q_OBJECT/Q_GADGET use as well as the inclusion of
# moc_*.cpp/*.moc. Rules are created to call moc accordingly at build time and
# add the generated sources to the target, if the generated code is not
# directly included otherwise.
function(qt_internal_automoc target)
    if ("x${ARGN}" STREQUAL "x")
        return()
    endif()

    if(NOT DEFINED QT_MOCSCANNER)
        get_target_property(mocPath "Qt::moc" LOCATION)
        get_filename_component(binDirectory "${mocPath}" DIRECTORY)
        set(QT_MOCSCANNER "${binDirectory}/qmocscanner${CMAKE_EXECUTABLE_SUFFIX}")
    endif()

    string(REPLACE ";" "\n" sources "${ARGN}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/moc_sources_and_headers.txt" ${sources})
    execute_process(COMMAND "${QT_MOCSCANNER}"
                    "${CMAKE_CURRENT_BINARY_DIR}/moc_sources_and_headers.txt"
                    "${CMAKE_CURRENT_BINARY_DIR}/moc_files_included.txt"
                    "${CMAKE_CURRENT_BINARY_DIR}/moc_files_to_build.txt"
                    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                    )

    file(STRINGS "${CMAKE_CURRENT_BINARY_DIR}/moc_files_included.txt" moc_files_included)
    file(STRINGS "${CMAKE_CURRENT_BINARY_DIR}/moc_files_to_build.txt" moc_files_to_build)

    file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/moc_sources_and_headers.txt")
    file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/moc_files_included.txt")
    file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/moc_files_to_build.txt")

    qt_internal_wrap_cpp("${target}" MOC ${moc_files_included} HEADER_FILE_ONLY ON)
    qt_internal_wrap_cpp("${target}" MOC ${moc_files_to_build} HEADER_FILE_ONLY OFF)
endfunction()


# This function takes a target as a parameter, followed by a list of sources.
# Any sources with the .ui extension are passed on to uic and the generated output
# is added to the target sources.
function(qt_internal_autouic target)
    set(outfiles "")

    get_target_property(source_dir "${target}" SOURCE_DIR)
    get_target_property(binary_dir "${target}" BINARY_DIR)

    foreach(infile ${ARGN})
        get_filename_component(ext "${infile}" EXT)
        if("${ext}" STREQUAL ".ui")
            qt_make_output_file("${infile}" "ui_" ".h" "${source_dir}" "${binary_dir}" outfile)
            qt_create_uic_command("${infile}" "${source_dir}" "${outfile}")
            list(APPEND outfiles "${outfile}")

            get_filename_component(outfile_path "${outfile}" PATH)
            target_include_directories("${target}" PRIVATE "${outfile_path}")
        endif()
    endforeach()
    target_sources("${target}" PRIVATE "${outfiles}")
endfunction()


# This function attempts to (poorly) parse the given resourceFile (.qrc) and
# determine the dependencies, i.e. which files are intended for inclusion into
# the Qt resource.
function(qt_extract_qrc_dependencies resourceFile out_depends_ rc_depends_)
    get_filename_component(rc_path ${resourceFile} PATH)

    if(EXISTS "${infile}")
        #  parse file for dependencies
        #  all files are absolute paths or relative to the location of the qrc file
        file(READ "${infile}" RC_FILE_CONTENTS)
        string(REGEX MATCHALL "<file[^<]+" RC_FILES "${RC_FILE_CONTENTS}")
        set(RC_DEPENDS "")
        foreach(RC_FILE ${RC_FILES})
            string(REGEX REPLACE "^<file[^>]*>" "" RC_FILE "${RC_FILE}")
            if(NOT IS_ABSOLUTE "${RC_FILE}")
                set(RC_FILE "${rc_path}/${RC_FILE}")
            endif()
            set(RC_DEPENDS ${RC_DEPENDS} "${RC_FILE}")
        endforeach()
        # Since this cmake function is doing the dependency scanning for these files,
        # let's make a configured file and add it as a dependency so cmake is run
        # again when dependencies need to be recomputed.
        qt_make_output_file("${infile}" "" ".qrc.depends" "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}" out_depends)
        configure_file("${infile}" "${out_depends}" COPYONLY)
    else()
        # The .qrc file does not exist (yet). Let's add a dependency and hope
        # that it will be generated later
        set(out_depends)
    endif()

    set(${out_depends_} ${out_depends} PARENT_SCOPE)
    set(${rc_depends_} ${RC_DEPENDS} PARENT_SCOPE)
endfunction()


# This function creates the necessary rule to call rcc on the given
# resource file and stores the name of the to-be generated C++ source
# file (created by rcc) in the outCppFile variable.
function(qt_create_rcc_command resourceFile source_dir binary_dir outfile)
    qt_extract_qrc_dependencies("${infile}" out_depends rc_depends "${source_dir}" "${binary_dir}")
    set_source_files_properties("${infile}" PROPERTIES SKIP_AUTORCC ON)

    get_filename_component(outfilename "${resourceFile}" NAME_WE)
    add_custom_command(OUTPUT "${outfile}"
                       COMMAND "Qt::rcc" --name "${outfilename}" --output "${outfile}" "${resourceFile}"
                       MAIN_DEPENDENCY "${infile}"
                       DEPENDS "${rc_depends}" "${out_depends}"
                       WORKING_DIRECTORY "${source_dir}" VERBATIM)
    set_source_files_properties("${outfile}" PROPERTIES SKIP_AUTOMOC ON)
    set_source_files_properties("${outfile}" PROPERTIES SKIP_AUTOUIC ON)
endfunction()


# This function takes a target as a parameter, followed by a list of sources.
# Any sources ending with the .qrc extension are treated as Qt resources and rules
# to call rcc are generated. The source files rcc generates are added to the target.
function(qt_internal_autorcc target)
    get_target_property(binary_dir "${target}" BINARY_DIR)
    get_target_property(source_dir "${target}" SOURCE_DIR)

    set(qrc_outfiles "")

    foreach(infile ${ARGN})
        get_filename_component(ext "${infile}" EXT)
        if("${ext}" STREQUAL ".qrc")
            qt_make_output_file("${infile}" "qrc_" ".cpp" "${source_dir}" "${binary_dir}" outfile)
            list(FIND all_sources "${outfile}" known_result)
            if (known_result GREATER -1)
                continue()
            endif()

            qt_create_rcc_command("${infile}" "${source_dir}" "${binary_dir}" "${outfile}")
            list(APPEND qrc_outfiles "${outfile}")
        endif()
    endforeach()
    target_sources("${target}" PRIVATE "${qrc_outfiles}")
endfunction()


# This function takes a target as a parameter, followed by a list of sources.
# The sources are scanned for .ui and .qrc as well as Q_OBJECT/Q_GADGET use
# and rules to call uic/rcc/moc are created. Any generated sources are added
# as private sources to the specified target.
function(qt_internal_process_automatic_sources target)
    qt_internal_automoc("${target}" ${ARGN})
    qt_internal_autouic("${target}" ${ARGN})
    qt_internal_autorcc("${target}" ${ARGN})
endfunction()


set(__default_private_args "SOURCES;LIBRARIES;INCLUDE_DIRECTORIES;DEFINES;DBUS_ADAPTOR_FLAGS;DBUS_ADAPTOR_SOURCES;DBUS_INTERFACE_FLAGS;DBUS_INTERFACE_SOURCES")
set(__default_public_args "PUBLIC_LIBRARIES;PUBLIC_INCLUDE_DIRECTORIES;PUBLIC_DEFINES")


# This function can be used to add sources/libraries/etc. to the specified CMake target
# if the provided CONDITION evaluates to true.
function(extend_target target)
    if (NOT TARGET "${target}")
        message(FATAL_ERROR "Trying to extend non-existing target \"${target}\".")
    endif()
    qt_parse_all_arguments(arg "extend_target" "" ""
        "CONDITION;${__default_public_args};${__default_private_args};COMPILE_FLAGS" ${ARGN})
    if ("x${arg_CONDITION}" STREQUAL x)
        set(arg_CONDITION ON)
    endif()

    qt_evaluate_config_expression(result ${arg_CONDITION})
    if (${result})
        set(dbus_sources "")
        foreach(adaptor ${arg_DBUS_ADAPTOR_SOURCES})
            qt_create_qdbusxml2cpp_command("${target}" "${adaptor}" ADAPTOR FLAGS "${arg_DBUS_ADAPTOR_FLAGS}")
            list(APPEND dbus_sources "${sources}")
        endforeach()
        foreach(interface ${arg_DBUS_INTERFACE_SOURCES})
            qt_create_qdbusxml2cpp_command("${target}" "${interface}" INTERFACE FLAGS "${arg_DBUS_INTERFACE_FLAGS}")
            list(APPEND dbus_sources "${sources}")
        endforeach()
        qt_internal_process_automatic_sources("${target}" "${arg_SOURCES}")

        set(must_push_features OFF)
        foreach(dep ${arg_LIBRARIES} ${arg_PUBLIC_LIBRARIES})
            if("${dep}" MATCHES "^Qt::((.+)(Private)|(.+))$")
                if (${CMAKE_MATCH_COUNT} EQUAL 3)
                    set(depTarget ${CMAKE_MATCH_2})
                else()
                    set(depTarget ${CMAKE_MATCH_4})
                endif()

                # Fetch features from dependencies and make them available to the
                # caller as well as to the local scope for configure.cmake evaluation.
                if(NOT TARGET "${dep}")
                    find_package(Qt${PROJECT_VERSION_MAJOR}${depTarget} REQUIRED)
                endif()

                if("x${CMAKE_MATCH_3}" STREQUAL "xPrivate")
                    qt_pull_features_into_current_scope(PRIVATE_FEATURES ${depTarget})
                endif()
                qt_pull_features_into_current_scope(PUBLIC_FEATURES ${depTarget})

                set(must_push_features ON)
            endif()
        endforeach()

        target_sources("${target}" PRIVATE ${arg_SOURCES} ${dbus_sources})
        if (arg_COMPILE_FLAGS)
            set_source_files_properties(${arg_SOURCES} PROPERTIES COMPILE_FLAGS "${arg_COMPILE_FLAGS}")
        endif()
        target_include_directories("${target}" PUBLIC ${arg_PUBLIC_INCLUDE_DIRECTORIES} PRIVATE ${arg_INCLUDE_DIRECTORIES})
        target_compile_definitions("${target}" PUBLIC ${arg_PUBLIC_DEFINES} PRIVATE ${arg_DEFINES})
        target_link_libraries("${target}" PUBLIC ${arg_PUBLIC_LIBRARIES} PRIVATE ${arg_LIBRARIES})

        if(must_push_features)
            qt_push_features_into_parent_scope()
        endif()
    endif()
endfunction()


function(qt_internal_library_deprecation_level result)
    if(WIN32)
        # On Windows, due to the way DLLs work, we need to export all functions,
        # including the inlines
        set("${result}" "QT_DISABLE_DEPRECATED_BEFORE=0x040800" PARENT_SCOPE)
    else()
        # On other platforms, Qt's own compilation goes needs to compile the Qt 5.0 API
        set("${result}" "QT_DISABLE_DEPRECATED_BEFORE=0x050000" PARENT_SCOPE)
    endif()
endfunction()


function(qt_read_headers_pri module resultVarPrefix)
    qt_internal_module_include_dir(include_dir "${module}")
    file(STRINGS "${include_dir}/headers.pri" headers_pri_contents)
    foreach(line ${headers_pri_contents})
        if("${line}" MATCHES "SYNCQT.HEADER_FILES = (.*)")
            set(public_module_headers "${CMAKE_MATCH_1}")
            separate_arguments(public_module_headers UNIX_COMMAND "${public_module_headers}")
        elseif("${line}" MATCHES "SYNCQT.PRIVATE_HEADER_FILES = (.*)")
            set(private_module_headers "${CMAKE_MATCH_1}")
            separate_arguments(private_module_headers UNIX_COMMAND "${private_module_headers}")
        elseif("${line}" MATCHES "SYNCQT.GENERATED_HEADER_FILES = (.*)")
            set(generated_module_headers "${CMAKE_MATCH_1}")
            separate_arguments(generated_module_headers UNIX_COMMAND "${generated_module_headers}")
            foreach(generated_header ${generated_module_headers})
                list(APPEND public_module_headers "${include_dir}/${generated_header}")
            endforeach()
        # Ignore INJECTIONS!
        elseif("${line}" MATCHES "SYNCQT.([A-Z_]+)_HEADER_FILES = (.+)")
            set(prefix "${CMAKE_MATCH_1}")
            string(TOLOWER "${prefix}" prefix)
            set(entries "${CMAKE_MATCH_2}")
            separate_arguments(entries UNIX_COMMAND "${entries}")
            set("${resultVarPrefix}_${prefix}" "${entries}" PARENT_SCOPE)
        endif()
    endforeach()
    set(${resultVarPrefix}_public "${public_module_headers}" PARENT_SCOPE)
    set(${resultVarPrefix}_private "${private_module_headers}" PARENT_SCOPE)
endfunction()


# Add Qt::target and Qt6::target as aliases for the target
function(qt_internal_add_target_aliases target)
    get_target_property(type "${target}" TYPE)
    if (type STREQUAL EXECUTABLE)
        add_executable("Qt::${target}" ALIAS "${target}")
        add_executable("Qt${PROJECT_VERSION_MAJOR}::${target}" ALIAS "${target}")
    else()
        add_library("Qt::${target}" ALIAS "${target}")
        add_library("Qt${PROJECT_VERSION_MAJOR}::${target}" ALIAS "${target}")
    endif()
endfunction()


# This is the main entry function for creating a Qt module, that typically
# consists of a library, public header files, private header files and configurable
# features.
#
# A CMake target with the specified target parameter is created. If the current source
# directory has a configure.cmake file, then that is also processed for feature definition
# and testing. Any features defined as well as any features coming from dependencies to
# this module are imported into the scope of the calling feature.
#
# Target is without leading "Qt". So e.g. the "QtCore" module has the target "Core".
function(add_qt_module target)
    qt_parse_all_arguments(arg "add_qt_module" "NO_MODULE_HEADERS;STATIC" "CONFIG_MODULE_NAME"
        "${__default_private_args};${__default_public_args};FEATURE_DEPENDENCIES" ${ARGN})

    qt_internal_module_info(module "${target}")

    _set_known_qt_modules("${KNOWN_QT_MODULES}" "${target}")

    ### Define Targets:
    if(${arg_STATIC})
        add_library("${target}" STATIC)
    elseif(${QT_BUILD_SHARED_LIBS})
        add_library("${target}" SHARED)
    else()
        add_library("${target}" STATIC)
    endif()
    qt_internal_add_target_aliases("${target}")

    # Add _private target to link against the private headers:
    set(target_private "${target}Private")
    add_library("${target_private}" INTERFACE)
    qt_internal_add_target_aliases("${target_private}")

    if(NOT DEFINED arg_CONFIG_MODULE_NAME)
        set(arg_CONFIG_MODULE_NAME "${module_lower}")
    endif()

    # Import global features
    if(NOT "${target}" STREQUAL "Core")
        qt_pull_features_into_current_scope(PUBLIC_FEATURES PRIVATE_FEATURES FEATURE_PROPERTY_INFIX "GLOBAL_" Qt::Core)
    endif()

    # Fetch features from dependencies and make them available to the
    # caller as well as to the local scope for configure.cmake evaluation.
    foreach(dep ${arg_LIBRARIES} ${arg_PUBLIC_LIBRARIES} ${arg_FEATURE_DEPENDENCIES})
        if("${dep}" MATCHES "(Qt::.+)Private")
            set(publicDep ${CMAKE_MATCH_1})
            qt_pull_features_into_current_scope(PRIVATE_FEATURES ${publicDep})
        else()
            set(publicDep ${dep})
        endif()
        qt_pull_features_into_current_scope(PUBLIC_FEATURES ${publicDep})
    endforeach()

    if(${arg_NO_MODULE_HEADERS})
        set_target_properties("${target}" PROPERTIES MODULE_HAS_HEADERS OFF)
    else()
        qt_ensure_perl()
        if(NOT DEFINED QT_SYNCQT)
            get_target_property(mocPath "Qt::moc" LOCATION)
            get_filename_component(binDirectory "${mocPath}" DIRECTORY)
            # We could put this into the cache, but on the other hand there's no real need to
            # pollute the app's cache with this. For the first qtbase build, the variable is
            # set in global scope.
            set(QT_SYNCQT "${binDirectory}/syncqt.pl")
        endif()
        execute_process(COMMAND "${HOST_PERL}" -w "${QT_SYNCQT}" -quiet -module "${module}" -version "${PROJECT_VERSION}" -outdir "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}")

        set_target_properties("${target}" PROPERTIES MODULE_HAS_HEADERS ON)
    endif()

    set_target_properties("${target}" PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${INSTALL_LIBDIR}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${INSTALL_BINDIR}"
        OUTPUT_NAME "${module_versioned}")

    qt_internal_module_include_dir(include_dir "${module}")

    set(configureFile "${CMAKE_CURRENT_SOURCE_DIR}/configure.cmake")
    if(EXISTS "${configureFile}")
        qt_feature_module_begin(
            LIBRARY "${module}"
            PUBLIC_FILE "qt${arg_CONFIG_MODULE_NAME}-config.h"
            PRIVATE_FILE "qt${arg_CONFIG_MODULE_NAME}-config_p.h"
            PUBLIC_DEPENDENCIES ${arg_FEATURE_DEPENDENCIES}
            PRIVATE_DEPENDENCIES ${arg_FEATURE_DEPENDENCIES}
        )
        include(${configureFile})
        qt_feature_module_end("${target}")
        qt_pull_features_into_current_scope(PUBLIC_FEATURES PRIVATE_FEATURES "${target}")

        set_property(TARGET "${target}" APPEND PROPERTY PUBLIC_HEADER "${CMAKE_CURRENT_BINARY_DIR}/qt${arg_CONFIG_MODULE_NAME}-config.h")
        set_property(TARGET "${target}" APPEND PROPERTY PRIVATE_HEADER "${CMAKE_CURRENT_BINARY_DIR}/qt${arg_CONFIG_MODULE_NAME}-config_p.h")

        if("${target}" STREQUAL "Core")
           set_property(TARGET "${target}" APPEND PROPERTY PUBLIC_HEADER "${CMAKE_CURRENT_BINARY_DIR}/global/qconfig.h")
           set_property(TARGET "${target}" APPEND PROPERTY PRIVATE_HEADER "${CMAKE_CURRENT_BINARY_DIR}/global/qconfig_p.h")
        endif()
    endif()

    qt_internal_library_deprecation_level(deprecation_define)

    extend_target("${target}"
        SOURCES ${arg_SOURCES}
        PUBLIC_INCLUDE_DIRECTORIES
            $<BUILD_INTERFACE:${include_dir}>
            $<INSTALL_INTERFACE:include/${module}>
            ${arg_PUBLIC_INCLUDE_DIRECTORIES}
        INCLUDE_DIRECTORIES
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}"
            $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
            "${include_dir}/${PROJECT_VERSION}"
            "${include_dir}/${PROJECT_VERSION}/${module}"
            ${arg_INCLUDE_DIRECTORIES}
        PUBLIC_DEFINES
            ${arg_PUBLIC_DEFINES}
            QT_${module_upper}_LIB
        DEFINES
            ${arg_DEFINES}
            QT_NO_CAST_TO_ASCII QT_ASCII_CAST_WARNINGS
            QT_MOC_COMPAT #we don't need warnings from calling moc code in our generated code
            QT_USE_QSTRINGBUILDER
            QT_DEPRECATED_WARNINGS
            QT_BUILDING_QT
            QT_BUILD_${module_upper}_LIB ### FIXME: use QT_BUILD_ADDON for Add-ons or remove if we don't have add-ons anymore
            "${deprecation_define}"
        PUBLIC_LIBRARIES
            ${arg_PUBLIC_LIBRARIES}
        LIBRARIES
            ${arg_LIBRARIES}
        DBUS_ADAPTOR_SOURCES "${arg_DBUS_ADAPTOR_SOURCES}"
        DBUS_ADAPTOR_FLAGS "${arg_DBUS_ADAPTOR_FLAGS}"
        DBUS_INTERFACE_SOURCES "${arg_DBUS_INTERFACE_SOURCES}"
        DBUS_INTERFACE_FLAGS "${arg_DBUS_INTERFACE_FLAGS}"
    )

    ### FIXME: Can we replace headers.pri?
    qt_internal_module_include_dir(include_dir "${module}")
    if(NOT ${arg_NO_MODULE_HEADERS})
        qt_read_headers_pri("${module}" "module_headers")
        set_property(TARGET "${target}" APPEND PROPERTY PUBLIC_HEADER "${module_headers_public}")
        set_property(TARGET "${target}" APPEND PROPERTY PRIVATE_HEADER "${module_headers_private}")
        set_property(TARGET "${target}" APPEND PROPERTY PUBLIC_HEADER "${include_dir}/${module}Depends")
    endif()

    if(DEFINED module_headers_private)
        qt_internal_add_linker_version_script("${target}" PRIVATE_HEADERS ${module_headers_private})
    else()
        qt_internal_add_linker_version_script("${target}")
    endif()

    qt_pull_features_into_current_scope(PUBLIC_FEATURES PRIVATE_FEATURES ${arg_FEATURE_DEPENDENCIES})

    install(TARGETS "${target}" "${target_private}" EXPORT "${module_versioned}Targets"
        LIBRARY DESTINATION ${INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${INSTALL_INCLUDEDIR}/${module}
        PRIVATE_HEADER DESTINATION ${INSTALL_INCLUDEDIR}/${module}/${PROJECT_VERSION}/${module}/private
        )

    set(config_install_dir "${INSTALL_LIBDIR}/cmake/${module_versioned}")
    install(EXPORT "${module_versioned}Targets" NAMESPACE Qt:: DESTINATION ${config_install_dir})

    configure_package_config_file(
        "${Qt${PROJECT_VERSION_MAJOR}_DIR}/QtModuleConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${module_versioned}Config.cmake"
        INSTALL_DESTINATION "${config_install_dir}"
    )
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/${module_versioned}ConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
    )

    set(extra_cmake_files)
    if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/${module_versioned}Macros.cmake")
        list(APPEND extra_cmake_files "${CMAKE_CURRENT_LIST_DIR}/${module_versioned}Macros.cmake")
    endif()

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${module_versioned}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${module_versioned}ConfigVersion.cmake"
        ${extra_cmake_files}
        DESTINATION "${config_install_dir}"
        COMPONENT Devel
    )

    ### fixme: cmake is missing a built-in variable for this. We want to apply it only to modules and plugins
    # that belong to Qt.
    if (GCC)
        qt_internal_add_link_flags("${target}" "-Wl,--no-undefined")
    endif()

    # When a public module depends on private, also make its private depend on the other's private
    set(qt_libs_private "")
    foreach(it ${KNOWN_QT_MODULES})
        list(FIND arg_LIBRARIES "Qt::${it}Private" pos)
        if(pos GREATER -1)
            list(APPEND qt_libs_private "Qt::${it}Private")
        endif()
    endforeach()

    target_link_libraries("${target_private}" INTERFACE "${target}" "${qt_libs_private}")

    qt_internal_module_include_dir(include_dir "${module}")
    target_include_directories("${target_private}" INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${include_dir}/${PROJECT_VERSION}>
        $<BUILD_INTERFACE:${include_dir}/${PROJECT_VERSION}/${module}>
        $<INSTALL_INTERFACE:include/${module}/${PROJECT_VERSION}>
        $<INSTALL_INTERFACE:include/${module}/${PROJECT_VERSION}/${module}>
    )

    qt_push_features_into_parent_scope()
endfunction()


# This is the main entry point for defining Qt plugins.
# A CMake target is created with the given name. The TYPE parameter is needed to place the
# plugin into the correct plugins/ sub-directory.
function(add_qt_plugin name)
    # This is a copy paste of add_qt_module with minor changes and some commented out stuff.
    # FIXME !!!
    set(module "${name}")
    string(TOUPPER "${name}" name_upper)

    qt_parse_all_arguments(arg "add_qt_plugin" "" "TYPE" "${__default_private_args};${__default_public_args}" ${ARGN})
    if (NOT DEFINED arg_TYPE)
        message(FATAL_ERROR "add_qt_plugin called without setting a TYPE.")
    endif()

    add_library("${module}")
    set_target_properties("${module}" PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${INSTALL_PLUGINSDIR}/${arg_TYPE}")
    set_target_properties("${module}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${INSTALL_BINDIR}")

    # Import global features
    qt_pull_features_into_current_scope(PUBLIC_FEATURES PRIVATE_FEATURES FEATURE_PROPERTY_INFIX "GLOBAL_" Qt::Core)

    # Fetch features from dependencies and make them available to the
    # caller as well as to the local scope for configure.cmake evaluation.
    foreach(dep ${arg_LIBRARIES} ${arg_PUBLIC_LIBRARIES})
        if("${dep}" MATCHES "(Qt::.+)Private")
            set(publicDep ${CMAKE_MATCH_1})
            qt_pull_features_into_current_scope(PRIVATE_FEATURES ${publicDep})
        else()
            set(publicDep ${dep})
        endif()
        qt_pull_features_into_current_scope(PUBLIC_FEATURES ${publicDep})
    endforeach()

    qt_internal_library_deprecation_level(deprecation_define)

    extend_target("${module}"
        SOURCES ${arg_SOURCES}
        INCLUDE_DIRECTORIES
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}"
            ${arg_INCLUDE_DIRECTORIES}
        PUBLIC_INCLUDE_DIRECTORIES ${arg_PUBLIC_INCLUDE_DIRECTORIES}
        LIBRARIES ${arg_LIBRARIES}
        PUBLIC_LIBRARIES ${arg_PUBLIC_LIBRARIES}
        DEFINES
            ${arg_DEFINES}
            QT_NO_CAST_TO_ASCII QT_ASCII_CAST_WARNINGS
            QT_MOC_COMPAT #we don't need warnings from calling moc code in our generated code
            QT_USE_QSTRINGBUILDER
            QT_DEPRECATED_WARNINGS
            QT_BUILDING_QT
            QT_BUILD_${name_upper}_LIB ### FIXME: use QT_BUILD_ADDON for Add-ons or remove if we don't have add-ons anymore
            "${deprecation_define}"
        PUBLIC_DEFINES
            QT_${name_upper}_LIB
            ${arg_PUBLIC_DEFINES}
        DBUS_ADAPTOR_SOURCES "${arg_DBUS_ADAPTOR_SOURCES}"
        DBUS_ADAPTOR_FLAGS "${arg_DBUS_ADAPTOR_FLAGS}"
        DBUS_INTERFACE_SOURCES "${arg_DBUS_INTERFACE_SOURCES}"
        DBUS_INTERFACE_FLAGS "${arg_DBUS_INTERFACE_FLAGS}"
    )

    install(TARGETS "${module}" EXPORT "${module}Targets"
        LIBRARY DESTINATION ${INSTALL_PLUGINSDIR}/${arg_TYPE}
        ARCHIVE DESTINATION ${INSTALL_LIBDIR}/${arg_TYPE}
        )
    install(EXPORT "${module}Targets" NAMESPACE Qt:: DESTINATION ${INSTALL_LIBDIR}/cmake)

    ### fixme: cmake is missing a built-in variable for this. We want to apply it only to modules and plugins
    # that belong to Qt.
    if (GCC)
        qt_internal_add_link_flags("${module}" "-Wl,--no-undefined")
    endif()

    qt_internal_add_linker_version_script(${module})

    qt_push_features_into_parent_scope()
endfunction()


# This function creates a CMake target for a generic console or GUI binary.
# Please consider to use a more specific version target like the one created
# by add_qt_test or add_qt_tool below.
function(add_qt_executable name)
    qt_parse_all_arguments(arg "add_qt_executable" "GUI" "OUTPUT_DIRECTORY" "EXE_FLAGS;${__default_private_args}" ${ARGN})

    if ("x${arg_OUTPUT_DIRECTORY}" STREQUAL "x")
        set(arg_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${INSTALL_BINDIR}")
    endif()

    add_executable("${name}" ${arg_EXE_FLAGS})
    extend_target("${name}"
        SOURCES ${arg_SOURCES}
        INCLUDE_DIRECTORIES
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}"
            ${arg_INCLUDE_DIRECTORIES}
        DEFINES ${arg_DEFINES}
        LIBRARIES ${arg_LIBRARIES}
        DBUS_ADAPTOR_SOURCES "${arg_DBUS_ADAPTOR_SOURCES}"
        DBUS_ADAPTOR_FLAGS "${arg_DBUS_ADAPTOR_FLAGS}"
        DBUS_INTERFACE_SOURCES "${arg_DBUS_INTERFACE_SOURCES}"
        DBUS_INTERFACE_FLAGS "${arg_DBUS_INTERFACE_FLAGS}"
    )
    set_target_properties("${name}" PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}"
        WIN32_EXECUTABLE "${arg_GUI}"
        MACOSX_BUNDLE "${arg_GUI}"
    )

    qt_push_features_into_parent_scope()
endfunction()


# This function creates a CMake test target with the specified name for use with CTest.
function(add_qt_test name)
    qt_parse_all_arguments(arg "add_qt_test" "RUN_SERIAL" "" "${__default_private_args}" ${ARGN})
    set(path "${CMAKE_CURRENT_BINARY_DIR}")

    add_qt_executable("${name}"
        OUTPUT_DIRECTORY "${path}"
        SOURCES "${arg_SOURCES}"
        INCLUDE_DIRECTORIES
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}"
            "${arg_INCLUDE_DIRECTORIES}"
        DEFINES "${arg_DEFINES}"
        LIBRARIES Qt::Core Qt::Test ${arg_LIBRARIES}
    )

    add_test(NAME "${name}" COMMAND "${name}" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

    set_tests_properties("${name}" PROPERTIES RUN_SERIAL "${arg_RUN_SERIAL}")
    set_property(TEST "${name}" APPEND PROPERTY ENVIRONMENT "PATH=${path}${QT_PATH_SEPARATOR}${CMAKE_CURRENT_BINARY_DIR}${QT_PATH_SEPARATOR}$ENV{PATH}")
    set_property(TEST "${name}" APPEND PROPERTY ENVIRONMENT "QT_PLUGIN_PATH=${PROJECT_BINARY_DIR}/${INSTALL_PLUGINSDIR}")

    qt_push_features_into_parent_scope()
endfunction()


# This function creates an executable for use as helper program with tests. Some
# tests launch separate programs to test certainly input/output behavior.
function(add_qt_test_helper name)
    add_qt_executable("${name}" OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" ${ARGN})
    qt_push_features_into_parent_scope()
endfunction()


# This function is used to define a "Qt tool", such as moc, uic or rcc.
# The BOOTSTRAP option allows building it as standalone program, otherwise
# it will be linked against QtCore.
function(add_qt_tool name)
    qt_parse_all_arguments(arg "add_qt_tool" "BOOTSTRAP" "" "${__default_private_args}" ${ARGN})

    if (arg_BOOTSTRAP)
        set(corelib Qt::Bootstrap)
    else()
        set(corelib Qt::Core)
    endif()

    add_qt_executable("${name}" OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${INSTALL_BINDIR}"
        # Do not pass sources here: They may not get processed when BOOTSTRAP is set!
        INCLUDE_DIRECTORIES
            ${arg_INCLUDE_DIRECTORIES}
        DEFINES ${arg_DEFINES}
        LIBRARIES ${corelib} ${arg_LIBRARIES}
    )
    target_sources("${name}" PRIVATE "${arg_SOURCES}")
    qt_internal_add_target_aliases("${name}")

    if (NOT arg_BOOTSTRAP)
        qt_internal_process_automatic_sources("${name}" ${arg_SOURCES})
    endif()

    install(TARGETS "${name}" EXPORT "Qt${PROJECT_VERSION_MAJOR}ToolsTargets" DESTINATION ${INSTALL_TARGETS_DEFAULT_ARGS})
    qt_push_features_into_parent_scope()
endfunction()



function(qt_create_tracepoints name tracePointsFile)
    #### TODO
    string(TOLOWER "${name}" name)

    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/qt${name}_tracepoints_p.h" CONTENT
        "#include <private/qtrace_p.h>")
endfunction()



function(add_qt_resource target resourceName)
    qt_parse_all_arguments(rcc "add_qt_resource" "" "PREFIX;BASE" "FILES" ${ARGN})

    set(qrcContents "<RCC>")
    if (${rcc_PREFIX})
        string(APPEND qrcContents "  <qresource>\n")
    else()
        string(APPEND qrcContents "  <qresource prefix=\"${rcc_PREFIX}\">\n")
    endif()

    foreach(file ${rcc_FILES})
        get_property(alias SOURCE ${file} PROPERTY alias)
        if (NOT alias)
            set(alias "${file}")
        endif()
        ### FIXME: escape file paths to be XML conform
        string(APPEND qrcContents "    <file alias=\"${alias}\">${CMAKE_CURRENT_SOURCE_DIR}/${file}</file>\n")
    endforeach()

    string(APPEND qrcContents "  </qresource>\n</RCC>\n")
    set(generatedResourceFile "${CMAKE_CURRENT_BINARY_DIR}/${resourceName}.qrc")
    file(GENERATE OUTPUT "${generatedResourceFile}" CONTENT "${qrcContents}")

    set(generatedSourceCode "${CMAKE_CURRENT_BINARY_DIR}/qrc_${resourceName}.cpp")
    add_custom_command(OUTPUT "${generatedSourceCode}"
                       COMMAND "Qt::rcc"
                       ARGS --name "${resourceName}"
                           --output "${generatedSourceCode}" "${generatedResourceFile}"
                       DEPENDS ${files}
                       COMMENT "RCC ${resourceName}"
                       VERBATIM)
    target_sources(${target} PRIVATE "${generatedSourceCode}")
endfunction()

# From Qt5CoreMacros
# Function used to create the names of output files preserving relative dirs
function(qt_make_output_file infile prefix suffix source_dir binary_dir result)
    get_filename_component(outfilename "${infile}" NAME_WE)

    set(base_dir "${source_dir}")
    string(FIND "${infile}" "${binary_dir}/" in_binary)
    if (in_binary EQUAL 0)
        set(base_dir "${binary_dir}")
    endif()

    get_filename_component(abs_infile "${infile}" ABSOLUTE BASE_DIR "${base_dir}")
    file(RELATIVE_PATH rel_infile "${base_dir}" "${abs_infile}")
    string(REPLACE "../" "__/" mapped_infile "${rel_infile}")

    get_filename_component(abs_mapped_infile "${mapped_infile}" ABSOLUTE BASE_DIR "${binary_dir}")
    get_filename_component(outpath "${abs_mapped_infile}" PATH)

    file(MAKE_DIRECTORY "${outpath}")
    set("${result}" "${outpath}/${prefix}${outfilename}${suffix}" PARENT_SCOPE)
endfunction()


macro(qt_get_moc_flags moc_flags)
    set(${moc_flags})
    get_directory_property(inc_DIRS INCLUDE_DIRECTORIES)

    if(CMAKE_INCLUDE_CURRENT_DIR)
        list(APPEND inc_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
    endif()

    foreach(current ${inc_DIRS})
        if("${current}" MATCHES "\\.framework/?$")
            string(REGEX REPLACE "/[^/]+\\.framework" "" framework_path "${current}")
            set(${moc_flags} ${${moc_flags}} "-F${framework_path}")
        else()
            set(${moc_flags} ${${moc_flags}} "-I${current}")
        endif()
    endforeach()

    get_directory_property(defines COMPILE_DEFINITIONS)
    foreach(current ${defines})
        set(${moc_flags} ${${moc_flags}} "-D${current}")
    endforeach()

    if(WIN32)
        set(${moc_flags} ${${moc_flags}} -DWIN32)
    endif()
    if (MSVC)
        set(${moc_flags} ${${moc_flags}} --compiler-flavor=msvc)
    endif()
endmacro()


# helper to set up a moc rule
function(qt_create_moc_command target source_dir infile outfile moc_flags moc_options moc_depends)
    # Pass the parameters in a file.  Set the working directory to
    # be that containing the parameters file and reference it by
    # just the file name.  This is necessary because the moc tool on
    # MinGW builds does not seem to handle spaces in the path to the
    # file given with the @ syntax.
    set (moc_parameters_file "${outfile}_parameters")
    set (moc_parameters ${moc_flags} ${moc_options} -o "${outfile}" "${infile}")
    string (REPLACE ";" "\n" moc_parameters "${moc_parameters}")

    set(moc_parameters_file "${moc_parameters_file}$<$<BOOL:$<CONFIGURATION>>:_$<CONFIGURATION>>")
    set(targetincludes "$<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>")
    set(targetdefines "$<TARGET_PROPERTY:${target},COMPILE_DEFINITIONS>")
    set(targetincludes "$<$<BOOL:${targetincludes}>:-I$<JOIN:${targetincludes},\n-I>\n>")
    set(targetdefines "$<$<BOOL:${targetdefines}>:-D$<JOIN:${targetdefines},\n-D>\n>")

    file (GENERATE
        OUTPUT "${moc_parameters_file}"
        CONTENT "${targetdefines}${targetincludes}${moc_parameters}\n"
    )

    set(moc_extra_parameters_file @${moc_parameters_file})
    add_custom_command(OUTPUT "${outfile}"
                       COMMAND "Qt::moc" "${moc_extra_parameters_file}"
                       DEPENDS "${infile}" ${moc_depends}
                       WORKING_DIRECTORY "${source_dir}" VERBATIM)
endfunction()


# helper to set up a uic rule
function(qt_create_uic_command infile source_dir outfile)
    add_custom_command(OUTPUT "${outfile}"
                       COMMAND "Qt::uic" "${infile}" -o "${outfile}"
                       DEPENDS "${infile}"
                       COMMENT "Running UIC on ${infile}."
                       WORKING_DIRECTORY "${source_dir}" VERBATIM)
    set_source_files_properties("${outfile}" PROPERTIES HEADER_FILE_ONLY ON)
endfunction()


# helper to set up a qdbusxml2cpp rule
function(qt_create_qdbusxml2cpp_command target infile)
    qt_parse_all_arguments(arg "qt_create_qdbusxml2cpp_command" "ADAPTOR;INTERFACE" "" "FLAGS" ${ARGN})
    if((arg_ADAPTOR AND arg_INTERFACE) OR (NOT arg_ADAPTOR AND NOT arg_INTERFACE))
        message(FATAL_ERROR "qt_create_dbusxml2cpp_command needs either ADAPTOR or INTERFACE.")
    endif()

    set(option "-a")
    set(type "adaptor")
    if (arg_INTERFACE)
        set(option "-p")
        set(type "interface")
    endif()

    get_filename_component(file_name "${infile}" NAME_WE)
    string(TOLOWER "${file_name}" file_name)

    set(header_file "${CMAKE_CURRENT_BINARY_DIR}/${file_name}_${type}.h")
    set(source_file "${CMAKE_CURRENT_BINARY_DIR}/${file_name}_${type}.cpp")

    add_custom_command(OUTPUT "${header_file}" "${source_file}"
                       COMMAND Qt::qdbusxml2cpp ${arg_FLAGS} "${option}" "${header_file}:${source_file}" "${infile}"
                       DEPENDS "${infile}"
                       WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                       VERBATIM)

    # Moc the header:
    qt_internal_wrap_cpp("${target}" MOC "${header_file}" HEADER_FILE_ONLY OFF)

    target_sources("${target}" PRIVATE "${header_file}" "${source_file}" "${moc_sources}")
endfunction()


function(qt_generate_forwarding_headers module)
    qt_parse_all_arguments(arg "qt_generate_forwarding_headers"
                           "PRIVATE" "SOURCE;DESTINATION" "CLASSES" ${ARGN})
    qt_internal_module_include_dir(include_dir "${module}")

    if (NOT arg_DESTINATION)
        get_filename_component(arg_DESTINATION "${arg_SOURCE}" NAME)
    endif()

    if (arg_PRIVATE)
        set(main_fwd "${include_dir}/${PROJECT_VERSION}/${module}/private/${arg_DESTINATION}")
    else()
        set(main_fwd "${include_dir}/${arg_DESTINATION}")
    endif()

    get_filename_component(main_fwd_dir "${main_fwd}" DIRECTORY)
    file(RELATIVE_PATH relpath "${main_fwd_dir}" "${CMAKE_CURRENT_BINARY_DIR}/${arg_SOURCE}")
    set(main_contents "#include \"${relpath}\"")
    file(GENERATE OUTPUT "${main_fwd}" CONTENT "${main_contents}")

    foreach(class_fwd ${arg_CLASSES})
        set(class_fwd_contents "#include \"${fwd_hdr}\"")
        message("Generating forwarding header: ${class_fwd} -> ${relpath}.")
        file(GENERATE OUTPUT "${include_dir}/${class_fwd}" CONTENT "${class_fwd_contents}")
    endforeach()
endfunction()


function(add_qt_docs qdocFile)
    # TODO
endfunction()
