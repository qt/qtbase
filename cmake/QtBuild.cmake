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

set(INSTALL_CMAKE_NAMESPACE "Qt${PROJECT_VERSION_MAJOR}" CACHE STRING "CMake namespace [Qt${PROJECT_VERSION_MAJOR}]")

set(QT_CMAKE_EXPORT_NAMESPACE "Qt${PROJECT_VERSION_MAJOR}" CACHE STRING "CMake namespace used when exporting targets [Qt${PROJECT_VERSION_MAJOR}]")
set(QT_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

# the default RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
   SET(_default_install_rpath "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${isSystemDir}" STREQUAL "-1")

# Default rpath settings: Use rpath for build tree as well as a full path for the installed binaries.
# For origin builds, one needs to override CMAKE_INSTALL_RPATH for example with $ORIGIN/../lib
SET(CMAKE_INSTALL_RPATH "${_default_install_rpath}" CACHE PATH "RPATH for installed binaries")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

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
if(QT_NAMESPACE STREQUAL "")
    set(QT_HAS_NAMESPACE OFF)
else()
    set(QT_HAS_NAMESPACE ON)
endif()

macro(qt_internal_set_qt_known_modules)
    set(QT_KNOWN_MODULES ${ARGN} CACHE INTERNAL "Known Qt modules" FORCE)
endmacro()

# Reset:
qt_internal_set_qt_known_modules("")

set(QT_KNOWN_MODULES_WITH_TOOLS "" CACHE INTERNAL "Known Qt modules with tools" FORCE)
macro(qt_internal_append_known_modules_with_tools module)
    if(NOT ${module} IN_LIST QT_KNOWN_MODULES_WITH_TOOLS)
        set(QT_KNOWN_MODULES_WITH_TOOLS "${QT_KNOWN_MODULES_WITH_TOOLS};${module}"
            CACHE INTERNAL "Known Qt modules with tools" FORCE)
    endif()
endmacro()

macro(qt_internal_append_known_module_tool module tool)
    if(NOT ${tool} IN_LIST QT_KNOWN_MODULE_${module}_TOOLS)
        list(APPEND QT_KNOWN_MODULE_${module}_TOOLS "${tool}")
        set(QT_KNOWN_MODULE_${module}_TOOLS "${QT_KNOWN_MODULE_${module}_TOOLS}"
            CACHE INTERNAL "Known Qt module ${module} tools" FORCE)
    endif()
endmacro()

# Reset syncqt cache variable, to make sure it gets recomputed on reconfiguration, otherwise
# it might not get installed.
unset(QT_SYNCQT CACHE)

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


function(qt_create_nolink_target target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "${target} does not exist when trying to build a nolink target.")
    endif()
    get_target_property(type "${target}" TYPE)
    if(type STREQUAL EXECUTABLE)
        message(FATAL_ERROR "${target} must be a library of some kind.")
    endif()
    if(type STREQUAL OBJECT_LIBRARY)
        message(FATAL_ERROR "${target} must not be an object library.")
    endif()

    set(nolink_target "${target}_nolink")
    if(NOT TARGET "${nolink_target}")
        add_library("${nolink_target}" INTERFACE IMPORTED)
        set_target_properties("${nolink_target}" PROPERTIES
                              INTERFACE_INCLUDE_DIRECTORIES
                              $<TARGET_PROPERTY:${target},INTERFACE_INCLUDE_DIRECTORIES>
                              INTERFACE_SYSTEM_INCLUDE_DIRECTORIES
                              $<TARGET_PROPERTY:${target},INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
                              INTERFACE_COMPILE_DEFINITIONS
                              $<TARGET_PROPERTY:${target},INTERFACE_COMPILE_DEFINITIONS>
                              INTERFACE_COMPILE_OPTIONS
                              $<TARGET_PROPERTY:${target},INTERFACE_COMPILE_OPTIONS>
                              INTERFACE_COMPILE_FEATURES
                              $<TARGET_PROPERTY:${target},INTERFACE_COMPILE_FEATURES>)
    endif()
endfunction()

function(qt_ensure_perl)
    if(DEFINED HOST_PERL)
        return()
    endif()
    find_program(HOST_PERL "perl" DOC "Perl binary")
    if (NOT HOST_PERL)
        message(FATAL_ERROR "Perl needs to be available to build Qt.")
    endif()
endfunction()


function(qt_ensure_sync_qt)
    qt_ensure_perl()
    if(DEFINED QT_SYNCQT)
        return()
    endif()

    # When building qtbase, use the source syncqt, otherwise use the installed one.
    if(EXISTS "${PROJECT_SOURCE_DIR}/bin/syncqt.pl")
        set(QT_SYNCQT "${PROJECT_SOURCE_DIR}/bin/syncqt.pl" CACHE FILEPATH "syncqt script")
        message(STATUS "Using source syncqt found at: ${QT_SYNCQT}")
        install(PROGRAMS "${PROJECT_SOURCE_DIR}/bin/syncqt.pl" DESTINATION "${INSTALL_LIBEXECDIR}")
    else()
        get_filename_component(syncqt_absolute_path
                               "${CMAKE_INSTALL_PREFIX}/${INSTALL_LIBEXECDIR}/syncqt.pl"
                               ABSOLUTE)
        set(QT_SYNCQT "${syncqt_absolute_path}" CACHE FILEPATH "syncqt script")
        message(STATUS "Using installed syncqt found at: ${QT_SYNCQT}")
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


function(qt_internal_add_link_flags_no_undefined target)
    if (GCC OR CLANG)
        if(APPLE)
            set(no_undefined_flag "-Wl,-undefined,error")
        elseif(LINUX)
            set(no_undefined_flag "-Wl,--no-undefined")
        else()
            message(FATAL_ERROR "Platform linker doesn't support erroring upon encountering undefined symbols. Target:\"${target}\".")
        endif()
        target_link_options("${target}" PRIVATE "${no_undefined_flag}")
    endif()
endfunction()

function(qt_internal_add_link_flags_gc_sections target visibility)
    set(possible_visibilities PRIVATE INTERFACE PUBLIC)
    list(FIND possible_visibilities "${visibility}" known_visibility)
    if (known_visibility EQUAL "-1")
        message(FATAL_ERROR "Visibitily setting must be one of PRIVATE, INTERFACE or PUBLIC.")
    endif()

    if (GCC OR CLANG)
        if(APPLE)
            set(gc_sections_flag "-Wl,-dead_strip")
        elseif(LINUX OR BSD OR SOLARIS OR WIN32)
            set(gc_sections_flag "-Wl,--gc-sections")
        else()
            message(FATAL_ERROR "Platform linker doesn't support gc sections. Target:\"${target}\".")
        endif()
        target_link_options("${target}" ${visibility} "${gc_sections_flag}")
    endif()
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
            target_link_options("${target}" PRIVATE "-Wl,--version-script,${outfile}")
        endif()
    endif()
endfunction()


# Get a set of Qt module related values based on the target name.
# When doing qt_internal_module_info(foo Core) this method will set
# the following variables in the caller's scope:
#  * foo with the value "QtCore"
#  * foo_versioned with the value "Qt5Core" (based on major Qt version)
#  * foo_upper with the value "CORE"
#  * foo_lower with the value "core"
#  * foo_include_dir with the module's include directory in the binary tree
function(qt_internal_module_info result target)
    set(module "Qt${target}")
    set("${result}" "${module}" PARENT_SCOPE)
    set("${result}_versioned" "Qt${PROJECT_VERSION_MAJOR}${target}" PARENT_SCOPE)
    string(TOUPPER "${target}" upper)
    string(TOLOWER "${target}" lower)
    set("${result}_upper" "${upper}" PARENT_SCOPE)
    set("${result}_lower" "${lower}" PARENT_SCOPE)
    set("${result}_include_dir" "${PROJECT_BINARY_DIR}/include/${module}" PARENT_SCOPE)
endfunction()


set(__default_private_args "SOURCES;LIBRARIES;INCLUDE_DIRECTORIES;DEFINES;DBUS_ADAPTOR_BASENAME;DBUS_ADAPTOR_FLAGS;DBUS_ADAPTOR_SOURCES;DBUS_INTERFACE_BASENAME;DBUS_INTERFACE_FLAGS;DBUS_INTERFACE_SOURCES;FEATURE_DEPENDENCIES;COMPILE_OPTIONS;LINK_OPTIONS;MOC_OPTIONS;DISABLE_AUTOGEN_TOOLS")
set(__default_public_args "PUBLIC_LIBRARIES;PUBLIC_INCLUDE_DIRECTORIES;PUBLIC_DEFINES;PUBLIC_COMPILE_OPTIONS;PUBLIC_LINK_OPTIONS")


option(QT_CMAKE_DEBUG_EXTEND_TARGET "Debug extend_target calls in Qt's build system" OFF)

# This function checks which autotools should be used: AUTOMOC/UIC/RCC
function(qt_autogen_tools target)
  qt_parse_all_arguments(arg "qt_autogen_tools" "" "" "${__default_private_args}" ${ARGN})

  set_property(TARGET "${target}" PROPERTY INTERFACE_QT_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
  set_property(TARGET "${target}" APPEND PROPERTY
    COMPATIBLE_INTERFACE_STRING QT_MAJOR_VERSION
  )

  list(APPEND autogen_tools "moc" "uic" "rcc")
  if (arg_DISABLE_AUTOGEN_TOOLS)
    foreach(disable_tool ${arg_DISABLE_AUTOGEN_TOOLS})
      list(REMOVE_ITEM autogen_tools "${disable_tool}")
    endforeach()
  endif()

  foreach(autogen_tool ${autogen_tools})
    string(TOUPPER "${autogen_tool}" captitalAutogenTool)

    set_target_properties("${target}"
                          PROPERTIES
                          AUTO${captitalAutogenTool} ON
                          AUTO${captitalAutogenTool}_EXECUTABLE
                          "$<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::${autogen_tool}>")
    set_property(TARGET ${target} APPEND PROPERTY
        AUTOGEN_TARGET_DEPENDS
        ${QT_CMAKE_EXPORT_NAMESPACE}::${autogen_tool})
  endforeach()

  set_directory_properties(PROPERTIES
                           QT_VERSION_MAJOR ${PROJECT_VERSION_MAJOR}
                           QT_VERSION_MINOR ${PROJECT_VERSION_MINOR})
endfunction()

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
        if(QT_CMAKE_DEBUG_EXTEND_TARGET)
            message("extend_target(${target} CONDITION ${arg_CONDITION} ...): Evaluated")
        endif()
        set(dbus_sources "")
        foreach(adaptor ${arg_DBUS_ADAPTOR_SOURCES})
            qt_create_qdbusxml2cpp_command("${target}" "${adaptor}" ADAPTOR BASENAME "${arg_DBUS_ADAPTOR_BASENAME}" FLAGS "${arg_DBUS_ADAPTOR_FLAGS}")
            list(APPEND dbus_sources "${sources}")
        endforeach()

        foreach(interface ${arg_DBUS_INTERFACE_SOURCES})
            qt_create_qdbusxml2cpp_command("${target}" "${interface}" INTERFACE BASENAME "${arg_DBUS_INTERFACE_BASENAME}" FLAGS "${arg_DBUS_INTERFACE_FLAGS}")
            list(APPEND dbus_sources "${sources}")
        endforeach()

        foreach(lib ${arg_PUBLIC_LIBRARIES} ${arg_LIBRARIES})
            string(REGEX REPLACE "_nolink$" "" base_lib "${lib}")
            if(NOT base_lib STREQUAL lib)
                qt_create_nolink_target("${base_lib}")
            endif()
        endforeach()

        # Find dependencies to internal libraries
        get_target_property(target_deps "${target}" _qt_target_deps)
        if(NOT target_deps)
            set(target_deps "")
        endif()

        foreach(lib ${arg_LIBRARIES})
            if (TARGET "${lib}")
                get_target_property(_is_exported "${lib}" INTERFACE_QT_EXPORTED_LIBRARY)
                if("${_is_exported}")
                    list(APPEND target_deps "${lib}\;${PROJECT_VERSION}")
                endif()
            endif()
        endforeach()

        # Set-up the target
        target_sources("${target}" PRIVATE ${arg_SOURCES} ${dbus_sources})
        if (arg_COMPILE_FLAGS)
            set_source_files_properties(${arg_SOURCES} PROPERTIES COMPILE_FLAGS "${arg_COMPILE_FLAGS}")
        endif()
        target_include_directories("${target}" PUBLIC ${arg_PUBLIC_INCLUDE_DIRECTORIES} PRIVATE ${arg_INCLUDE_DIRECTORIES})
        target_compile_definitions("${target}" PUBLIC ${arg_PUBLIC_DEFINES} PRIVATE ${arg_DEFINES})
        target_link_libraries("${target}" PUBLIC ${arg_PUBLIC_LIBRARIES} PRIVATE ${arg_LIBRARIES})
        target_compile_options("${target}" PUBLIC ${arg_PUBLIC_COMPILE_OPTIONS} PRIVATE ${arg_COMPILE_OPTIONS})
        target_link_options("${target}" PUBLIC ${arg_PUBLIC_LINK_OPTIONS} PRIVATE ${arg_LINK_OPTIONS})

        set_target_properties("${target}" PROPERTIES
            AUTOMOC_MOC_OPTIONS "${arg_MOC_OPTIONS}"
            _qt_target_deps "${target_deps}"
        )
    else()
        if(QT_CMAKE_DEBUG_EXTEND_TARGET)
            message("extend_target(${target} CONDITION ${arg_CONDITION} ...): Skipped")
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


function(qt_install_injections module)
    set(injections ${ARGN})
    # examples:
    #  SYNCQT.INJECTIONS = src/corelib/global/qconfig.h:qconfig.h:QtConfig src/corelib/global/qconfig_p.h:5.12.0/QtCore/private/qconfig_p.h
    #  SYNCQT.INJECTIONS = src/gui/vulkan/qvulkanfunctions.h:^qvulkanfunctions.h:QVulkanFunctions:QVulkanDeviceFunctions src/gui/vulkan/qvulkanfunctions_p.h:^5.12.0/QtGui/private/qvulkanfunctions_p.h
    separate_arguments(injections UNIX_COMMAND "${injections}")
    foreach(injection ${injections})
        string(REPLACE ":" ";" injection ${injection})
        list(GET injection 0 file)
        list(GET injection 1 destination)
        string(REGEX REPLACE "^\\^" "" destination "${destination}")
        list(REMOVE_AT injection 0 1)
        set(fwd_hdrs ${injection})
        get_filename_component(destinationdir ${destination} DIRECTORY)
        get_filename_component(destinationname ${destination} NAME)
        install(FILES ${PROJECT_BINARY_DIR}/${file} DESTINATION ${INSTALL_INCLUDEDIR}/${module}/${destinationdir} RENAME ${destinationname} OPTIONAL)
        foreach(fwd_hdr ${fwd_hdrs})
            file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${fwd_hdr}" CONTENT "#include \"${destinationname}\"\n")
            install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${fwd_hdr}" DESTINATION ${INSTALL_INCLUDEDIR}/${module}/${destinationdir} OPTIONAL)
        endforeach()
    endforeach()
endfunction()


function(qt_read_headers_pri target resultVarPrefix)
    qt_internal_module_info(module "${target}")
    file(STRINGS "${module_include_dir}/headers.pri" headers_pri_contents)
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
                list(APPEND public_module_headers "${module_include_dir}/${generated_header}")
            endforeach()
        elseif("${line}" MATCHES "SYNCQT.INJECTIONS = (.*)")
            set(injections "${CMAKE_MATCH_1}")
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
    set(${resultVarPrefix}_injections "${injections}" PARENT_SCOPE)
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
    qt_internal_module_info(module "${target}")

    # Process arguments:
    qt_parse_all_arguments(arg "add_qt_module"
        "NO_MODULE_HEADERS;STATIC;DISABLE_TOOLS_EXPORT"
        "CONFIG_MODULE_NAME"
        "${__default_private_args};${__default_public_args}" ${ARGN})

    if(NOT DEFINED arg_CONFIG_MODULE_NAME)
        set(arg_CONFIG_MODULE_NAME "${module_lower}")
    endif()

    qt_internal_set_qt_known_modules("${QT_KNOWN_MODULES}" "${target}")

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

    # Module headers:
    if(${arg_NO_MODULE_HEADERS})
        set_target_properties("${target}" PROPERTIES MODULE_HAS_HEADERS OFF)
    else()
        qt_ensure_sync_qt()
        execute_process(COMMAND "${HOST_PERL}" -w "${QT_SYNCQT}" -quiet -module "${module}" -version "${PROJECT_VERSION}" -outdir "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}")

        set_target_properties("${target}" PROPERTIES MODULE_HAS_HEADERS ON)

        ### FIXME: Can we replace headers.pri?
        qt_read_headers_pri("${target}" "module_headers")
        set_property(TARGET "${target}" APPEND PROPERTY PUBLIC_HEADER "${module_headers_public}")
        set_property(TARGET "${target}" APPEND PROPERTY PUBLIC_HEADER "${module_include_dir}/${module}Depends")
        set_property(TARGET "${target}" APPEND PROPERTY PRIVATE_HEADER "${module_headers_private}")
        qt_install_injections("${module}" ${module_headers_injections})
    endif()

    set_target_properties("${target}" PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${INSTALL_LIBDIR}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${INSTALL_BINDIR}"
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        OUTPUT_NAME "${INSTALL_CMAKE_NAMESPACE}${target}")

    qt_internal_library_deprecation_level(deprecation_define)

    qt_autogen_tools("${target}"
      DISABLE_AUTOGEN_TOOLS ${arg_DISABLE_AUTOGEN_TOOLS}
    )

    extend_target("${target}"
        SOURCES ${arg_SOURCES}
        PUBLIC_INCLUDE_DIRECTORIES
            $<BUILD_INTERFACE:${module_include_dir}>
            $<INSTALL_INTERFACE:include/${module}>
            ${arg_PUBLIC_INCLUDE_DIRECTORIES}
        INCLUDE_DIRECTORIES
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}"
            $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
            "${module_include_dir}/${PROJECT_VERSION}"
            "${module_include_dir}/${PROJECT_VERSION}/${module}"
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
        PUBLIC_LIBRARIES ${arg_PUBLIC_LIBRARIES}
        LIBRARIES ${arg_LIBRARIES}
        FEATURE_DEPENDENCIES ${arg_FEATURE_DEPENDENCIES}
        DBUS_ADAPTOR_SOURCES ${arg_DBUS_ADAPTOR_SOURCES}
        DBUS_ADAPTOR_FLAGS ${arg_DBUS_ADAPTOR_FLAGS}
        DBUS_INTERFACE_SOURCES ${arg_DBUS_INTERFACE_SOURCES}
        DBUS_INTERFACE_FLAGS ${arg_DBUS_INTERFACE_FLAGS}
        COMPILE_OPTIONS ${arg_COMPILE_OPTIONS}
        PUBLIC_COMPILE_OPTIONS ${arg_PUBLIC_COMPILE_OPTIONS}
        LINK_OPTIONS ${arg_LINK_OPTIONS}
        PUBLIC_LINK_OPTIONS ${arg_PUBLIC_LINK_OPTIONS}
        MOC_OPTIONS ${arg_MOC_OPTIONS}
    )

    set(configureFile "${CMAKE_CURRENT_SOURCE_DIR}/configure.cmake")
    if(EXISTS "${configureFile}")
        qt_feature_module_begin(
            LIBRARY "${target}"
            PUBLIC_FILE "qt${arg_CONFIG_MODULE_NAME}-config.h"
            PRIVATE_FILE "qt${arg_CONFIG_MODULE_NAME}-config_p.h"
            PUBLIC_DEPENDENCIES ${arg_FEATURE_DEPENDENCIES}
            PRIVATE_DEPENDENCIES ${arg_FEATURE_DEPENDENCIES}
        )
        include(${configureFile})
        qt_feature_module_end("${target}")

        set_property(TARGET "${target}" APPEND PROPERTY PUBLIC_HEADER "${CMAKE_CURRENT_BINARY_DIR}/qt${arg_CONFIG_MODULE_NAME}-config.h")
        set_property(TARGET "${target}" APPEND PROPERTY PRIVATE_HEADER "${CMAKE_CURRENT_BINARY_DIR}/qt${arg_CONFIG_MODULE_NAME}-config_p.h")
    endif()

    if(DEFINED module_headers_private)
        qt_internal_add_linker_version_script("${target}" PRIVATE_HEADERS ${module_headers_private})
    else()
        qt_internal_add_linker_version_script("${target}")
    endif()

    install(TARGETS "${target}" "${target_private}" EXPORT "${INSTALL_CMAKE_NAMESPACE}${target}Targets"
        LIBRARY DESTINATION ${INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${INSTALL_INCLUDEDIR}/${module}
        PRIVATE_HEADER DESTINATION ${INSTALL_INCLUDEDIR}/${module}/${PROJECT_VERSION}/${module}/private
        )

    set(config_install_dir "${INSTALL_LIBDIR}/cmake/${INSTALL_CMAKE_NAMESPACE}${target}")
    install(EXPORT "${INSTALL_CMAKE_NAMESPACE}${target}Targets" NAMESPACE ${QT_CMAKE_EXPORT_NAMESPACE}:: DESTINATION ${config_install_dir})

    set(extra_cmake_files)
    set(extra_cmake_includes)
    if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}Macros.cmake")
        list(APPEND extra_cmake_files "${CMAKE_CURRENT_LIST_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}Macros.cmake")
        list(APPEND extra_cmake_includes "${INSTALL_CMAKE_NAMESPACE}${target}Macros.cmake")
    endif()
    if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigExtras.cmake.in")
        configure_file("${CMAKE_CURRENT_LIST_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigExtras.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigExtras.cmake"
            @ONLY)
        list(APPEND extra_cmake_files "${CMAKE_CURRENT_BINARY_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigExtras.cmake")
        list(APPEND extra_cmake_includes "${INSTALL_CMAKE_NAMESPACE}${target}ConfigExtras.cmake")
    endif()

    ### fixme: cmake is missing a built-in variable for this. We want to apply it only to modules and plugins
    # that belong to Qt.
    qt_internal_add_link_flags_no_undefined("${target}")

    # When a public module depends on private, also make its private depend on the other's private
    set(qt_libs_private "")
    foreach(it ${QT_KNOWN_MODULES})
        list(FIND arg_LIBRARIES "Qt::${it}Private" pos)
        if(pos GREATER -1)
            list(APPEND qt_libs_private "Qt::${it}Private")
        endif()
    endforeach()

    target_link_libraries("${target_private}" INTERFACE "${target}" "${qt_libs_private}")
    target_include_directories("${target_private}" INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${module_include_dir}/${PROJECT_VERSION}>
        $<BUILD_INTERFACE:${module_include_dir}/${PROJECT_VERSION}/${module}>
        $<INSTALL_INTERFACE:include/${module}/${PROJECT_VERSION}>
        $<INSTALL_INTERFACE:include/${module}/${PROJECT_VERSION}/${module}>
    )

    get_target_property(target_deps "${target}" _qt_target_deps)
    if(NOT target_deps)
        set(target_deps "")
    endif()

    # TODO: should this also be in extend_target ? From the looks of it I would say that
    # it is not necessary but I'm not sure
    foreach(lib IN LISTS arg_PUBLIC_LIBRARIES qt_libs_private)
        if ("${lib}" MATCHES "^Qt::(.*)")
            set(lib "${CMAKE_MATCH_1}")
            if (lib STREQUAL Platform OR lib STREQUAL GlobalConfig)
                list(APPEND target_deps "Qt5\;${PROJECT_VERSION}")
            elseif ("${lib}" MATCHES "(.*)Private")
                list(APPEND target_deps "${INSTALL_CMAKE_NAMESPACE}${CMAKE_MATCH_1}\;${PROJECT_VERSION}")
            else()
                list(APPEND target_deps "${INSTALL_CMAKE_NAMESPACE}${lib}\;${PROJECT_VERSION}")
            endif()
        endif()
    endforeach()

    set_target_properties("${target}" PROPERTIES _qt_target_deps "${target_deps}")

    configure_package_config_file(
        "${QT_CMAKE_DIR}/QtModuleConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}Config.cmake"
        INSTALL_DESTINATION "${config_install_dir}"
    )
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersion.cmake"
        ${extra_cmake_files}
        DESTINATION "${config_install_dir}"
        COMPONENT Devel
    )

    if(NOT ${arg_DISABLE_TOOLS_EXPORT})
        qt_export_tools(${target})
    endif()
endfunction()

function(qt_export_tools module_name)
    # If no tools were defined belonging to this module, don't create a config and targets file.
    # Guards against the case when doing a cross-build and the function is called manually and not
    # by add_qt_module.

    if(NOT "${module_name}" IN_LIST QT_KNOWN_MODULES_WITH_TOOLS)
        return()
    endif()

    # The tools target name. For example: CoreTools
    set(target "${module_name}Tools")
    set(config_install_dir "${INSTALL_LIBDIR}/cmake/${INSTALL_CMAKE_NAMESPACE}${target}")

    # Add the extra cmake statements to make the tool targets global, so it doesn't matter where
    # find_package is called.
    # Also assemble a list of tool targets to expose in the config file for informational purposes.
    set(extra_cmake_statements "")
    set(tool_targets "")
    foreach(tool_name ${QT_KNOWN_MODULE_${module_name}_TOOLS})
        set(extra_cmake_statements "${extra_cmake_statements}
get_property(is_global TARGET ${INSTALL_CMAKE_NAMESPACE}::${tool_name} PROPERTY IMPORTED_GLOBAL)
if(NOT is_global)
    set_property(TARGET ${INSTALL_CMAKE_NAMESPACE}::${tool_name} PROPERTY IMPORTED_GLOBAL TRUE)
endif()
")
        list(APPEND tool_targets "${QT_CMAKE_EXPORT_NAMESPACE}::${tool_name}")
    endforeach()

    string(APPEND extra_cmake_statements
"set(${QT_CMAKE_EXPORT_NAMESPACE}${module_name}Tools_TARGETS \"${tool_targets}\")")

    configure_package_config_file(
        "${QT_CMAKE_DIR}/QtModuleToolsConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}Config.cmake"
        INSTALL_DESTINATION "${config_install_dir}"
    )
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersion.cmake"
        DESTINATION "${config_install_dir}"
        COMPONENT Devel
    )

    install(EXPORT "${INSTALL_CMAKE_NAMESPACE}${target}Targets"
        NAMESPACE "${QT_CMAKE_EXPORT_NAMESPACE}::"
        DESTINATION "${INSTALL_LIBDIR}/cmake/${INSTALL_CMAKE_NAMESPACE}${target}")
endfunction()

function(qt_internal_check_directory_or_type name dir type default result_var)
    if ("x${dir}" STREQUAL x)
        if("x${type}" STREQUAL x)
            message(FATAL_ERROR "add_qt_plugin called without setting either TYPE or ${name}.")
        endif()
        set(${result_var} "${default}" PARENT_SCOPE)
    else()
        set(${result_var} "${dir}" PARENT_SCOPE)
    endif()
endfunction()

# This is the main entry point for defining Qt plugins.
# A CMake target is created with the given target. The TYPE parameter is needed to place the
# plugin into the correct plugins/ sub-directory.
function(add_qt_plugin target)
    qt_internal_module_info(module "${target}")

    qt_parse_all_arguments(arg "add_qt_plugin" "STATIC"
        "TYPE;OUTPUT_DIRECTORY;INSTALL_DIRECTORY;ARCHIVE_INSTALL_DIRECTORY"
        "${__default_private_args};${__default_public_args}" ${ARGN})

    qt_internal_check_directory_or_type(OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}" "${arg_TYPE}"
        "${CMAKE_BINARY_DIR}/${INSTALL_PLUGINSDIR}/${arg_TYPE}" output_directory)
    qt_internal_check_directory_or_type(INSTALL_DIRECTORY "${arg_INSTALL_DIRECTORY}" "${arg_TYPE}"
        "${INSTALL_PLUGINSDIR}/${arg_TYPE}" install_directory)
    qt_internal_check_directory_or_type(ARCHIVE_INSTALL_DIRECTORY
        "${arg_ARCHIVE_INSTALL_DIRECTORY}" "${arg_TYPE}"
        "${INSTALL_LIBDIR}/${arg_TYPE}" archive_install_directory)

    if(arg_STATIC)
        add_library("${target}" STATIC)
    else()
        add_library("${target}")
    endif()
    qt_internal_add_target_aliases("${target}")

    set_target_properties("${target}" PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${output_directory}"
        RUNTIME_OUTPUT_DIRECTORY "${output_directory}"
        ARCHIVE_OUTPUT_DIRECTORY "${output_directory}")

    qt_internal_library_deprecation_level(deprecation_define)

    qt_autogen_tools("${target}"
      DISABLE_AUTOGEN_TOOLS ${arg_DISABLE_AUTOGEN_TOOLS}
    )

    set(static_plugin_define "")
    if (arg_STATIC OR NOT QT_BUILD_SHARED_LIBS)
        set(static_plugin_define "QT_STATICPLUGIN")
    endif()

    extend_target("${target}"
        SOURCES ${arg_SOURCES}
        INCLUDE_DIRECTORIES
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}"
            # For the syncqt headers
            $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
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
            QT_BUILD_${module_upper}_LIB ### FIXME: use QT_BUILD_ADDON for Add-ons or remove if we don't have add-ons anymore
            "${deprecation_define}"
            "${static_plugin_define}"
            QT_PLUGIN
        PUBLIC_DEFINES
            QT_${module_upper}_LIB
            ${arg_PUBLIC_DEFINES}
        FEATURE_DEPENDENCIES ${arg_FEATURE_DEPENDENCIES}
        DBUS_ADAPTOR_SOURCES "${arg_DBUS_ADAPTOR_SOURCES}"
        DBUS_ADAPTOR_FLAGS "${arg_DBUS_ADAPTOR_FLAGS}"
        DBUS_INTERFACE_SOURCES "${arg_DBUS_INTERFACE_SOURCES}"
        DBUS_INTERFACE_FLAGS "${arg_DBUS_INTERFACE_FLAGS}"
        COMPILE_OPTIONS ${arg_COMPILE_OPTIONS}
        PUBLIC_COMPILE_OPTIONS ${arg_PUBLIC_COMPILE_OPTIONS}
        LINK_OPTIONS ${arg_LINK_OPTIONS}
        PUBLIC_LINK_OPTIONS ${arg_PUBLIC_LINK_OPTIONS}
        MOC_OPTIONS ${arg_MOC_OPTIONS}
    )

    install(TARGETS "${target}" EXPORT "${target}Targets"
        LIBRARY DESTINATION "${install_directory}"
        ARCHIVE DESTINATION "${archive_install_directory}")
    install(EXPORT "${target}Targets" NAMESPACE ${QT_CMAKE_EXPORT_NAMESPACE}:: DESTINATION ${INSTALL_LIBDIR}/cmake)

    ### fixme: cmake is missing a built-in variable for this. We want to apply it only to modules and plugins
    # that belong to Qt.
    qt_internal_add_link_flags_no_undefined("${target}")

    qt_internal_add_linker_version_script(${target})
endfunction()


# This function creates a CMake target for a generic console or GUI binary.
# Please consider to use a more specific version target like the one created
# by add_qt_test or add_qt_tool below.
function(add_qt_executable name)
    qt_parse_all_arguments(arg "add_qt_executable" "GUI;BOOTSTRAP;NO_QT;NO_INSTALL" "OUTPUT_DIRECTORY;INSTALL_DIRECTORY" "EXE_FLAGS;${__default_private_args}" ${ARGN})

    if ("x${arg_OUTPUT_DIRECTORY}" STREQUAL "x")
        set(arg_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${INSTALL_BINDIR}")
    endif()

    get_filename_component(arg_OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}"
        ABSOLUTE BASE_DIR "${CMAKE_BINARY_DIR}")

    if ("x${arg_INSTALL_DIRECTORY}" STREQUAL "x")
        set(arg_INSTALL_DIRECTORY "${INSTALL_BINDIR}")
    endif()

    add_executable("${name}" ${arg_EXE_FLAGS})

    qt_autogen_tools("${name}"
      DISABLE_AUTOGEN_TOOLS ${arg_DISABLE_AUTOGEN_TOOLS}
    )

    set(extra_libraries "")
    if(NOT arg_BOOTSTRAP AND NOT arg_NO_QT)
        set(extra_libraries "Qt::Core")
    endif()

    extend_target("${name}"
        SOURCES ${arg_SOURCES}
        INCLUDE_DIRECTORIES
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}"
            ${arg_INCLUDE_DIRECTORIES}
        DEFINES ${arg_DEFINES}
        LIBRARIES ${arg_LIBRARIES} ${extra_libraries}
        DBUS_ADAPTOR_SOURCES "${arg_DBUS_ADAPTOR_SOURCES}"
        DBUS_ADAPTOR_FLAGS "${arg_DBUS_ADAPTOR_FLAGS}"
        DBUS_INTERFACE_SOURCES "${arg_DBUS_INTERFACE_SOURCES}"
        DBUS_INTERFACE_FLAGS "${arg_DBUS_INTERFACE_FLAGS}"
        COMPILE_OPTIONS ${arg_COMPILE_OPTIONS}
        LINK_OPTIONS ${arg_LINK_OPTIONS}
        MOC_OPTIONS ${arg_MOC_OPTIONS}
    )
    set_target_properties("${name}" PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}"
        WIN32_EXECUTABLE "${arg_GUI}"
        MACOSX_BUNDLE "${arg_GUI}"
    )

    if(WIN32)
        # Workaround for not having ported the winmain / qtmain static library
        # which is always linked in by qmake.
        set_target_properties("${name}" PROPERTIES WIN32_EXECUTABLE 0)
    endif()

    if(NOT arg_NO_INSTALL)
        install(TARGETS "${name}"
            RUNTIME DESTINATION "${arg_INSTALL_DIRECTORY}"
            BUNDLE DESTINATION "${arg_INSTALL_DIRECTORY}")
    endif()
endfunction()


# This function creates a CMake test target with the specified name for use with CTest.
function(add_qt_test name)
    qt_parse_all_arguments(arg "add_qt_test" "RUN_SERIAL" "" "${__default_private_args}" ${ARGN})
    set(path "${CMAKE_CURRENT_BINARY_DIR}")

    add_qt_executable("${name}"
        NO_INSTALL
        OUTPUT_DIRECTORY "${path}"
        SOURCES "${arg_SOURCES}"
        INCLUDE_DIRECTORIES
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}"
            $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
            "${arg_INCLUDE_DIRECTORIES}"
        DEFINES "${arg_DEFINES}"
        LIBRARIES ${QT_CMAKE_EXPORT_NAMESPACE}::Core ${QT_CMAKE_EXPORT_NAMESPACE}::Test ${arg_LIBRARIES}
        COMPILE_OPTIONS ${arg_COMPILE_OPTIONS}
        LINK_OPTIONS ${arg_LINK_OPTIONS}
        MOC_OPTIONS ${arg_MOC_OPTIONS}
        DISABLE_AUTOGEN_TOOLS ${arg_DISABLE_AUTOGEN_TOOLS}
    )

    add_test(NAME "${name}" COMMAND "${name}" WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

    set_tests_properties("${name}" PROPERTIES RUN_SERIAL "${arg_RUN_SERIAL}")
    set_property(TEST "${name}" APPEND PROPERTY ENVIRONMENT "PATH=${path}${QT_PATH_SEPARATOR}${CMAKE_CURRENT_BINARY_DIR}${QT_PATH_SEPARATOR}$ENV{PATH}")
    set_property(TEST "${name}" APPEND PROPERTY ENVIRONMENT "QT_PLUGIN_PATH=${PROJECT_BINARY_DIR}/${INSTALL_PLUGINSDIR}")
endfunction()


# This function creates an executable for use as helper program with tests. Some
# tests launch separate programs to test certainly input/output behavior.
function(add_qt_test_helper name)
    add_qt_executable("${name}" NO_INSTALL OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.." ${ARGN})
endfunction()

# Sets QT_WILL_BUILD_TOOLS if tools will be built.
function(qt_check_if_tools_will_be_built)
    set01(will_build_tools NOT CMAKE_CROSSCOMPILING AND NOT QT_FORCE_FIND_TOOLS)
    set(QT_WILL_BUILD_TOOLS ${will_build_tools} CACHE INTERNAL "Are tools going to be built" FORCE)
endfunction()

# This function is used to define a "Qt tool", such as moc, uic or rcc.
# The BOOTSTRAP option allows building it as standalone program, otherwise
# it will be linked against QtCore.
function(add_qt_tool name)
    qt_parse_all_arguments(arg "add_qt_tool" "BOOTSTRAP;NO_QT;NO_INSTALL" "TOOLS_TARGET"
                               "${__default_private_args}" ${ARGN})

    # Handle case when a tool does not belong to a module and it can't be built either (like
    # during a cross-compile).
    if(NOT arg_TOOLS_TARGET AND NOT QT_WILL_BUILD_TOOLS)
        message(FATAL_ERROR "The tool \"${name}\" has not been assigned to a module via"
                            " TOOLS_TARGET (so it can't be found) and it can't be built"
                            " (QT_WILL_BUILD_TOOLS is ${QT_WILL_BUILD_TOOLS}).")
    endif()

    set(full_name "${QT_CMAKE_EXPORT_NAMESPACE}::${name}")
    if(TARGET ${full_name})
        get_property(path TARGET ${full_name} PROPERTY LOCATION)
        message(STATUS "Tool '${full_name}' was found at ${path}.")
        return()
    endif()

    if(arg_TOOLS_TARGET AND NOT QT_WILL_BUILD_TOOLS)
        set(tools_package_name "Qt5${arg_TOOLS_TARGET}Tools")
        message(STATUS "Searching for tool '${full_name}' in package ${tools_package_name}.")

        # Only search in path provided by QT_HOST_PATH. We need to do it with CMAKE_PREFIX_PATH
        # instead of PATHS option, because any find_dependency call inside a Tools package would
        # not get the proper prefix when using PATHS.
        set(BACKUP_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
        set(CMAKE_PREFIX_PATH "${QT_HOST_PATH}")

        # Search both with sysroots prepended as well as in the host system. When cross compiling
        # the mode_package might be set to ONLY only, and the Qt5 tools packages are actually
        # in the host system.
        set(BACKUP_CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ${CMAKE_FIND_ROOT_PATH_MODE_PACKAGE})
        set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE "BOTH")
        set(BACKUP_CMAKE_SIZEOF_VOID_P "${CMAKE_SIZEOF_VOID_P}")
        set(CMAKE_SIZEOF_VOID_P "")
        find_package(
            ${tools_package_name}
            ${PROJECT_VERSION}
            NO_PACKAGE_ROOT_PATH
            NO_CMAKE_ENVIRONMENT_PATH
            NO_SYSTEM_ENVIRONMENT_PATH
            NO_CMAKE_PACKAGE_REGISTRY
            NO_CMAKE_SYSTEM_PATH
            NO_CMAKE_SYSTEM_PACKAGE_REGISTRY)
        set(CMAKE_SIZEOF_VOID_P "${BACKUP_CMAKE_SIZEOF_VOID_P}")
        set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE "${BACKUP_CMAKE_FIND_ROOT_PATH_MODE_PACKAGE}")
        set(CMAKE_PREFIX_PATH "${BACKUP_CMAKE_PREFIX_PATH}")

        if(${${tools_package_name}_FOUND} AND TARGET ${full_name})
            get_property(path TARGET ${full_name} PROPERTY LOCATION)
            message(STATUS "${full_name} was found at ${path} using package ${tools_package_name}.")
            return()
        endif()
    endif()

    if(NOT QT_WILL_BUILD_TOOLS)
        message(FATAL_ERROR "The tool \"${full_name}\" was not found in the "
                           "${tools_package_name} package. "
                           "Package found: ${${tools_package_name}_FOUND}")
    else()
        message(STATUS "Tool '${full_name}' will be built from source.")
    endif()

    set(disable_autogen_tools "${arg_DISABLE_AUTOGEN_TOOLS}")
    if (arg_NO_QT)
        # FIXME: Remove NO_QT again once qmake can use a "normal" Qt!
        if (arg_BOOTSTRAP)
            message(FATAL_ERROR "Tool can not be NO_QT and BOOTSTRAP at the same time!")
        endif()
        set(corelib "")
    else()
        if (arg_BOOTSTRAP)
            set(corelib ${QT_CMAKE_EXPORT_NAMESPACE}::Bootstrap)
            list(APPEND disable_autogen_tools "uic" "moc" "rcc")
        else()
            set(corelib ${QT_CMAKE_EXPORT_NAMESPACE}::Core)
        endif()
    endif()

    set(bootstrap "")
    if(arg_BOOTSTRAP)
        set(bootstrap BOOTSTRAP)
    endif()

    set(no_qt "")
    if(arg_NO_QT)
        set(no_qt NO_QT)
    endif()

    set(no_install "")
    if(arg_NO_INSTALL)
        set(no_install NO_INSTALL)
    endif()

    add_qt_executable("${name}" OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${INSTALL_BINDIR}"
        ${bootstrap}
        ${no_qt}
        ${no_install}
        SOURCES ${arg_SOURCES}
        INCLUDE_DIRECTORIES
            ${arg_INCLUDE_DIRECTORIES}
        DEFINES ${arg_DEFINES}
        LIBRARIES ${corelib} ${arg_LIBRARIES}
        COMPILE_OPTIONS ${arg_COMPILE_OPTIONS}
        LINK_OPTIONS ${arg_LINK_OPTIONS}
        MOC_OPTIONS ${arg_MOC_OPTIONS}
        DISABLE_AUTOGEN_TOOLS ${disable_autogen_tools}
    )
    qt_internal_add_target_aliases("${name}")

    if(NOT arg_NO_INSTALL AND arg_TOOLS_TARGET)
        # Assign a tool to an export set, and mark the module to which the tool belongs.
        qt_internal_append_known_modules_with_tools("${arg_TOOLS_TARGET}")

        # Also append the tool to the module list.
        qt_internal_append_known_module_tool("${arg_TOOLS_TARGET}" "${name}")

        install(TARGETS "${name}"
                EXPORT "Qt${PROJECT_VERSION_MAJOR}${arg_TOOLS_TARGET}ToolsTargets"
                DESTINATION ${INSTALL_TARGETS_DEFAULT_ARGS})
    endif()
endfunction()



function(qt_create_tracepoints name tracePointsFile)
    #### TODO
    string(TOLOWER "${name}" name)

    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/qt${name}_tracepoints_p.h" CONTENT
        "#include <private/qtrace_p.h>")
endfunction()



function(add_qt_resource target resourceName)
    qt_parse_all_arguments(rcc "add_qt_resource" "" "PREFIX;LANG;BASE" "FILES" ${ARGN})

    # Generate .qrc file:

    # <RCC><qresource ...>
    set(qrcContents "<RCC>\n  <qresource")
    if (rcc_PREFIX)
        string(APPEND qrcContents " prefix=\"${rcc_PREFIX}\"")
    endif()
    if (rcc_LANG)
        string(APPEND qrcContents " lang=\"${rcc_LANG}\"")
    endif()
    string(APPEND qrcContents ">\n")

    foreach(file ${rcc_FILES})
        if(rcc_BASE)
            set(based_file "${rcc_BASE}/${file}")
        else()
            set(based_file "${file}")
        endif()
        get_property(alias SOURCE ${based_file} PROPERTY alias)
        if (NOT alias)
            set(alias "${file}")
        endif()
        ### FIXME: escape file paths to be XML conform
        # <file ...>...</file>
        string(APPEND qrcContents "    <file alias=\"${alias}\">")
        string(APPEND qrcContents "${CMAKE_CURRENT_SOURCE_DIR}/${based_file}</file>\n")
    endforeach()

    # </qresource></RCC>
    string(APPEND qrcContents "  </qresource>\n</RCC>\n")

    set(generatedResourceFile "${CMAKE_CURRENT_BINARY_DIR}/${resourceName}.qrc")
    file(GENERATE OUTPUT "${generatedResourceFile}" CONTENT "${qrcContents}")

    # Process .qrc file:

    set(generatedSourceCode "${CMAKE_CURRENT_BINARY_DIR}/qrc_${resourceName}.cpp")
    add_custom_command(OUTPUT "${generatedSourceCode}"
                       COMMAND "${QT_CMAKE_EXPORT_NAMESPACE}::rcc"
                       ARGS --name "${resourceName}"
                           --output "${generatedSourceCode}" "${generatedResourceFile}"
                       DEPENDS ${files}
                       COMMENT "RCC ${resourceName}"
                       VERBATIM)
    target_sources(${target} PRIVATE "${generatedSourceCode}")
endfunction()


# Handle files that need special SIMD-related flags.
# This creates an object library and makes target link
# to it (privately).
function(add_qt_simd_part target)
    qt_parse_all_arguments(arg "add_qt_simd_part" "" ""
       "NAME;SIMD;${__default_private_args};COMPILE_FLAGS" ${ARGN})
    if ("x${arg_SIMD}" STREQUAL x)
        message(FATAL_ERROR "add_qt_simd_part needs a SIMD type to be set.")
    endif()

    set(condition "QT_FEATURE_${arg_SIMD}")
    if("${arg_SIMD}" STREQUAL arch_haswell)
        set(condition "TEST_subarch_avx2 AND TEST_subarch_bmi AND TEST_subarch_bmi2 AND TEST_subarch_f16c AND TEST_subarch_fma AND TEST_subarch_lzcnt AND TEST_subarch_popcnt")
    elseif("${arg_SIMD}" STREQUAL avx512common)
        set(condition "TEST_subarch_avx512cd")
    elseif("${arg_SIMD}" STREQUAL avx512core)
        set(condition "TEST_subarch_avx512cd AND TEST_subarch_avx512bw AND TEST_subarch_avx512dq AND TEST_subarch_avx512vl")
    endif()

    set(name "${arg_NAME}")
    if("x${name}" STREQUAL x)
        set(name "${target}_simd_${arg_SIMD}")
    endif()

    qt_evaluate_config_expression(result ${condition})
    if(${result})
        if(QT_CMAKE_DEBUG_EXTEND_TARGET)
            message("add_qt_simd_part(${target} SIMD ${arg_SIMD} ...): Evaluated")
        endif()
        string(TOUPPER "QT_CFLAGS_${arg_SIMD}" simd_flags)

        add_library("${name}" OBJECT)
        target_sources("${name}" PRIVATE ${arg_SOURCES})
        target_include_directories("${name}" PRIVATE
            ${arg_INCLUDE_DIRECTORIES}
            $<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>)
        target_compile_options("${name}" PRIVATE
            ${${simd_flags}}
            ${arg_COMPILE_FLAGS}
            $<TARGET_PROPERTY:${target},COMPILE_OPTIONS>)
            target_compile_definitions("${name}" PRIVATE
            $<TARGET_PROPERTY:${target},COMPILE_DEFINITIONS>)

        target_link_libraries("${target}" PRIVATE "${name}")

        if(NOT BUILD_SHARED_LIBS)
            install(
              TARGETS ${name}
              EXPORT "${INSTALL_CMAKE_NAMESPACE}Targets"
            )
        endif()
    else()
        if(QT_CMAKE_DEBUG_EXTEND_TARGET)
            message("add_qt_simd_part(${target} SIMD ${arg_SIMD} ...): Skipped")
        endif()
    endif()
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


# Complete manual moc invocation with full control.
# Use AUTOMOC whenever possible.
function(qt_manual_moc result)
    cmake_parse_arguments(arg "" "" "FLAGS" ${ARGN})
    set(moc_files)
    foreach(infile ${arg_UNPARSED_ARGUMENTS})
        qt_make_output_file("${infile}" "moc_" ".cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}" outfile)
        list(APPEND moc_files "${outfile}")

        set(moc_parameters_file "${outfile}_parameters$<$<BOOL:$<CONFIGURATION>>:_$<CONFIGURATION>>")
        set(moc_parameters ${arg_FLAGS} -o "${outfile}" "${infile}")
        string (REPLACE ";" "\n" moc_parameters "${moc_parameters}")

        file(GENERATE OUTPUT "${moc_parameters_file}" CONTENT "${moc_parameters}\n")

        add_custom_command(OUTPUT "${outfile}"
                           COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::moc "@${moc_parameters_file}"
                           DEPENDS "${infile}" ${moc_depends} ${QT_CMAKE_EXPORT_NAMESPACE}::moc
                           WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" VERBATIM)
    endforeach()
    set("${result}" ${moc_files} PARENT_SCOPE)
endfunction()


# helper to set up a qdbusxml2cpp rule
function(qt_create_qdbusxml2cpp_command target infile)
    qt_parse_all_arguments(arg "qt_create_qdbusxml2cpp_command" "ADAPTOR;INTERFACE" "BASENAME" "FLAGS" ${ARGN})
    if((arg_ADAPTOR AND arg_INTERFACE) OR (NOT arg_ADAPTOR AND NOT arg_INTERFACE))
        message(FATAL_ERROR "qt_create_dbusxml2cpp_command needs either ADAPTOR or INTERFACE.")
    endif()

    set(option "-a")
    set(type "adaptor")
    if (arg_INTERFACE)
        set(option "-p")
        set(type "interface")
    endif()

    if ("${arg_BASENAME}" STREQUAL "")
        get_filename_component(file_dir "${infile}" DIRECTORY)
        get_filename_component(file_name "${infile}" NAME_WLE)
        get_filename_component(file_ext "${infile}" LAST_EXT)

        if("${file_ext}" STREQUAL ".xml")
        else()
            message(FATAL_ERROR "DBUS ${type} input file is not xml.")
        endif()

        # use last part of io.qt.something.xml!
        get_filename_component(file_ext "${file_name}" LAST_EXT)
        if("x${file_ext}" STREQUAL "x")
        else()
            string(SUBSTRING "${file_ext}" 1 -1 file_name) # cut of leading '.'
        endif()

        string(TOLOWER "${file_name}" file_name)
        set(file_name "${file_name}_${type}")
    else()
        set(file_name ${arg_BASENAME})
    endif()

    # Use absolute file path for the source file and set the current working directory to the
    # current binary directory, because setting an absolute path for the header:source combo option
    # does not work. Splitting on ":" breaks inside the dbus tool when running on Windows
    # due to ":" being contained in the drive path (e.g C:\foo.h:C:\foo.cpp).
    get_filename_component(absolute_in_file_path "${infile}" ABSOLUTE)

    set(header_file "${file_name}.h")
    set(source_file "${file_name}.cpp")

    add_custom_command(OUTPUT "${header_file}" "${source_file}"
                       COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::qdbusxml2cpp ${arg_FLAGS} "${option}"
                               "${header_file}:${source_file}" "${absolute_in_file_path}"
                       DEPENDS "${absolute_in_file_path}"
                       WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
                       VERBATIM)

    target_sources("${target}" PRIVATE "${header_file}" "${source_file}")
endfunction()


function(qt_generate_forwarding_headers target)
    qt_parse_all_arguments(arg "qt_generate_forwarding_headers"
                           "PRIVATE" "SOURCE;DESTINATION" "CLASSES" ${ARGN})
    qt_internal_module_info(module "${target}")

    if (NOT arg_DESTINATION)
        get_filename_component(arg_DESTINATION "${arg_SOURCE}" NAME)
    endif()

    if (arg_PRIVATE)
        set(main_fwd "${module_include_dir}/${PROJECT_VERSION}/${module}/private/${arg_DESTINATION}")
    else()
        set(main_fwd "${module_include_dir}/${arg_DESTINATION}")
    endif()

    get_filename_component(main_fwd_dir "${main_fwd}" DIRECTORY)
    file(RELATIVE_PATH relpath "${main_fwd_dir}" "${CMAKE_CURRENT_BINARY_DIR}/${arg_SOURCE}")
    set(main_contents "#include \"${relpath}\"")
    file(GENERATE OUTPUT "${main_fwd}" CONTENT "${main_contents}")

    foreach(class_fwd ${arg_CLASSES})
        set(class_fwd_contents "#include \"${fwd_hdr}\"")
        message("Generating forwarding header: ${class_fwd} -> ${relpath}.")
        file(GENERATE OUTPUT "${module_include_dir}/${class_fwd}" CONTENT "${class_fwd_contents}")
    endforeach()
endfunction()


function(add_qt_docs qdocFile)
    # TODO
endfunction()

macro(qt_find_package)
    # Get the target names we expect to be provided by the package.
    cmake_parse_arguments(arg "" "" "PROVIDED_TARGETS;COMPONENTS" ${ARGN})

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

    # Call original function without our custom arguments.
    find_package(${arg_UNPARSED_ARGUMENTS})

    if(${ARGV0}_FOUND AND arg_PROVIDED_TARGETS)
        # If package was found, associate each target with its package name. This will be used
        # later when creating Config files for Qt libraries, to generate correct find_dependency()
        # calls. Also make the provided targets global, so that the properties can be read in
        # all scopes.
        foreach(qt_find_package_target_name ${arg_PROVIDED_TARGETS})
            if(TARGET ${qt_find_package_target_name})
                set_target_properties(${qt_find_package_target_name}
                                      PROPERTIES INTERFACE_QT_PACKAGE_NAME ${ARGV0})
                if(package_version)
                    set_target_properties(${qt_find_package_target_name}
                                          PROPERTIES INTERFACE_QT_PACKAGE_VERSION ${ARGV1})
                endif()

                if(arg_COMPONENTS)
                    set_target_properties(${qt_find_package_target_name}
                                          PROPERTIES
                                          INTERFACE_QT_PACKAGE_COMPONENTS ${arg_COMPONENTS})
                endif()

                get_property(is_global TARGET ${qt_find_package_target_name} PROPERTY
                                                                             IMPORTED_GLOBAL)
                if(NOT is_global)
                    set_property(TARGET ${qt_find_package_target_name} PROPERTY
                                                                       IMPORTED_GLOBAL TRUE)
                endif()
            else()
                message(AUTHOR_WARNING
                        "Error while trying to mark target '${qt_find_package_target_name}' as part"
                        " of the ${ARGV0} package. Provided target name does not exist.")
            endif()

        endforeach()
    endif()
endmacro()

# Creates a simple export set for the various Find* dependencies
# which are needed when creating a static build of Qt.
# This introduces a custom target property: INTERFACE_QT_EXPORTED_LIBRARY
# This target property indicates that Qt modules / plugins using this 3rd party library
# must add it to their list of dependencies when creating their own ${qtmodule}Config.cmake
function(qt_install_static_target_export target)
    if(BUILD_SHARED_LIBS)
        return()
    endif()

    qt_parse_all_arguments(arg "qt_install_3rdparty_config_files" "" "EXPORT" "" ${ARGN})
    # TODO mark EXPORT as required

    set(config_install_dir "${INSTALL_LIBDIR}/cmake/${arg_EXPORT}")

    set_target_properties(${target}
        PROPERTIES
            INTERFACE_QT_EXPORTED_LIBRARY 1)

    install(
        TARGETS ${target}
        EXPORT ${arg_EXPORT}Targets
        LIBRARY DESTINATION ${INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${INSTALL_LIBDIR})

    install(
        EXPORT ${arg_EXPORT}Targets
        DESTINATION "${config_install_dir}"
    )

    export(EXPORT ${arg_EXPORT}Targets)
endfunction()

# Create a set of ${target}Config.cmake and ${target}Version.cmake for a
# third-party library so that it can be found by client code linking statically.
function(qt_install_3rdparty_config_files target)
    if(BUILD_SHARED_LIBS)
       return()
    endif()

    qt_parse_all_arguments(arg "qt_install_3rdparty_config_files" "" "EXPORT" "PACKAGES;ADDITIONAL_FILES" ${ARGN})
    # TODO mark EXPORT as required

    set(3RDPARTY_ADDITIONAL_SETUP_CODE)
    foreach(package ${arg_PACKAGES})
      list(APPEND 3RDPARTY_ADDITIONAL_SETUP_CODE "find_package(${package})\n")
    endforeach()

    set(config_install_dir "${INSTALL_LIBDIR}/cmake/${arg_EXPORT}")
    configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/3rdpartyConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${target}Config.cmake"
        INSTALL_DESTINATION "${config_install_dir}"
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${target}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${target}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${target}ConfigVersion.cmake"
        ${arg_ADDITIONAL_FILES}
        DESTINATION "${config_install_dir}"
        COMPONENT Devel
    )
endfunction()

# Call this function in 3rdparty find modules that ought to be installed alongside
# Qt modules and must be found when linking statically.
function(qt_install_3rdparty_library target)
    qt_install_static_target_export(${target} EXPORT ${target})
    qt_install_3rdparty_config_files(${target} EXPORT ${target} ${ARGN})
endfunction()

