#=============================================================================
# Copyright 2005-2011 Kitware, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of Kitware, Inc. nor the names of its
#   contributors may be used to endorse or promote products derived
#   from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

######################################
#
#       Macros for building Qt files
#
######################################

include(CMakeParseArguments)

set(__qt_core_macros_module_base_dir "${CMAKE_CURRENT_LIST_DIR}")

# macro used to create the names of output files preserving relative dirs
macro(_qt_internal_make_output_file infile prefix ext outfile )
    string(LENGTH ${CMAKE_CURRENT_BINARY_DIR} _binlength)
    string(LENGTH ${infile} _infileLength)
    set(_checkinfile ${CMAKE_CURRENT_SOURCE_DIR})
    if(_infileLength GREATER _binlength)
        string(SUBSTRING "${infile}" 0 ${_binlength} _checkinfile)
        if(_checkinfile STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
            file(RELATIVE_PATH rel ${CMAKE_CURRENT_BINARY_DIR} ${infile})
        else()
            file(RELATIVE_PATH rel ${CMAKE_CURRENT_SOURCE_DIR} ${infile})
        endif()
    else()
        file(RELATIVE_PATH rel ${CMAKE_CURRENT_SOURCE_DIR} ${infile})
    endif()
    if(WIN32 AND rel MATCHES "^([a-zA-Z]):(.*)$") # absolute path
        set(rel "${CMAKE_MATCH_1}_${CMAKE_MATCH_2}")
    endif()
    set(_outfile "${CMAKE_CURRENT_BINARY_DIR}/${rel}")
    string(REPLACE ".." "__" _outfile ${_outfile})
    get_filename_component(outpath ${_outfile} PATH)
    if(CMAKE_VERSION VERSION_LESS "3.14")
        get_filename_component(_outfile_ext ${_outfile} EXT)
        get_filename_component(_outfile_ext ${_outfile_ext} NAME_WE)
        get_filename_component(_outfile ${_outfile} NAME_WE)
        string(APPEND _outfile ${_outfile_ext})
    else()
        get_filename_component(_outfile ${_outfile} NAME_WLE)
    endif()
    file(MAKE_DIRECTORY ${outpath})
    set(${outfile} ${outpath}/${prefix}${_outfile}.${ext})
endmacro()

macro(_qt_internal_get_moc_flags _moc_flags)
    set(${_moc_flags})
    get_directory_property(_inc_DIRS INCLUDE_DIRECTORIES)

    if(CMAKE_INCLUDE_CURRENT_DIR)
        list(APPEND _inc_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
    endif()

    foreach(_current ${_inc_DIRS})
        if("${_current}" MATCHES "\\.framework/?$")
            string(REGEX REPLACE "/[^/]+\\.framework" "" framework_path "${_current}")
            set(${_moc_flags} ${${_moc_flags}} "-F${framework_path}")
        else()
            set(${_moc_flags} ${${_moc_flags}} "-I${_current}")
        endif()
    endforeach()

    get_directory_property(_defines COMPILE_DEFINITIONS)
    foreach(_current ${_defines})
        set(${_moc_flags} ${${_moc_flags}} "-D${_current}")
    endforeach()

    if(WIN32)
        set(${_moc_flags} ${${_moc_flags}} -DWIN32)
    endif()
    if (MSVC)
        set(${_moc_flags} ${${_moc_flags}} --compiler-flavor=msvc)
    endif()
endmacro()

# helper macro to set up a moc rule
function(_qt_internal_create_moc_command infile outfile moc_flags moc_options moc_target moc_depends)
    # Pass the parameters in a file.  Set the working directory to
    # be that containing the parameters file and reference it by
    # just the file name.  This is necessary because the moc tool on
    # MinGW builds does not seem to handle spaces in the path to the
    # file given with the @ syntax.
    get_filename_component(_moc_outfile_name "${outfile}" NAME)
    get_filename_component(_moc_outfile_dir "${outfile}" PATH)
    if(_moc_outfile_dir)
        set(_moc_working_dir WORKING_DIRECTORY ${_moc_outfile_dir})
    endif()
    set (_moc_parameters_file ${outfile}_parameters)
    set (_moc_parameters ${moc_flags} ${moc_options} -o "${outfile}" "${infile}")
    string (REPLACE ";" "\n" _moc_parameters "${_moc_parameters}")

    if(moc_target)
        set(_moc_parameters_file ${_moc_parameters_file}$<$<BOOL:$<CONFIGURATION>>:_$<CONFIGURATION>>)
        set(targetincludes "$<TARGET_PROPERTY:${moc_target},INCLUDE_DIRECTORIES>")
        set(targetdefines "$<TARGET_PROPERTY:${moc_target},COMPILE_DEFINITIONS>")

        set(targetincludes "$<$<BOOL:${targetincludes}>:-I$<JOIN:${targetincludes},\n-I>\n>")
        set(targetdefines "$<$<BOOL:${targetdefines}>:-D$<JOIN:${targetdefines},\n-D>\n>")

        file (GENERATE
            OUTPUT ${_moc_parameters_file}
            CONTENT "${targetdefines}${targetincludes}${_moc_parameters}\n"
        )

        set(targetincludes)
        set(targetdefines)
    else()
        file(WRITE ${_moc_parameters_file} "${_moc_parameters}\n")
    endif()

    set(_moc_extra_parameters_file @${_moc_parameters_file})
    add_custom_command(OUTPUT ${outfile}
                       COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::moc ${_moc_extra_parameters_file}
                       DEPENDS ${infile} ${moc_depends}
                       ${_moc_working_dir}
                       VERBATIM)
    set_source_files_properties(${infile} PROPERTIES SKIP_AUTOMOC ON)
    set_source_files_properties(${outfile} PROPERTIES SKIP_AUTOMOC ON)
    set_source_files_properties(${outfile} PROPERTIES SKIP_AUTOUIC ON)
endfunction()

function(qt6_generate_moc infile outfile )
    # get include dirs and flags
    _qt_internal_get_moc_flags(moc_flags)
    get_filename_component(abs_infile ${infile} ABSOLUTE)
    set(_outfile "${outfile}")
    if(NOT IS_ABSOLUTE "${outfile}")
        set(_outfile "${CMAKE_CURRENT_BINARY_DIR}/${outfile}")
    endif()
    if ("x${ARGV2}" STREQUAL "xTARGET")
        set(moc_target ${ARGV3})
    endif()
    _qt_internal_create_moc_command(${abs_infile} ${_outfile} "${moc_flags}" "" "${moc_target}" "")
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_generate_moc)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 5)
            qt5_generate_moc(${ARGV})
        elseif(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_generate_moc(${ARGV})
        endif()
    endfunction()
endif()


# qt6_wrap_cpp(outfiles inputfile ... )

function(qt6_wrap_cpp outfiles )
    # get include dirs
    _qt_internal_get_moc_flags(moc_flags)

    set(options)
    set(oneValueArgs TARGET)
    set(multiValueArgs OPTIONS DEPENDS)

    cmake_parse_arguments(_WRAP_CPP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(moc_files ${_WRAP_CPP_UNPARSED_ARGUMENTS})
    set(moc_options ${_WRAP_CPP_OPTIONS})
    set(moc_target ${_WRAP_CPP_TARGET})
    set(moc_depends ${_WRAP_CPP_DEPENDS})

    foreach(it ${moc_files})
        get_filename_component(it ${it} ABSOLUTE)
        _qt_internal_make_output_file(${it} moc_ cpp outfile)
        _qt_internal_create_moc_command(
            ${it} ${outfile} "${moc_flags}" "${moc_options}" "${moc_target}" "${moc_depends}")
        list(APPEND ${outfiles} ${outfile})
    endforeach()
    set(${outfiles} ${${outfiles}} PARENT_SCOPE)
endfunction()

# This will override the CMake upstream command, because that one is for Qt 3.
if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_wrap_cpp outfiles)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 5)
            qt5_wrap_cpp("${outfiles}" ${ARGN})
        elseif(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_wrap_cpp("${outfiles}" ${ARGN})
        endif()
        set("${outfiles}" "${${outfiles}}" PARENT_SCOPE)
    endfunction()
endif()


# _qt6_parse_qrc_file(infile _out_depends _rc_depends)
# internal

function(_qt6_parse_qrc_file infile _out_depends _rc_depends)
    get_filename_component(rc_path ${infile} PATH)

    if(EXISTS "${infile}")
        #  parse file for dependencies
        #  all files are absolute paths or relative to the location of the qrc file
        file(READ "${infile}" RC_FILE_CONTENTS)
        string(REGEX MATCHALL "<file[^<]+" RC_FILES "${RC_FILE_CONTENTS}")
        foreach(RC_FILE ${RC_FILES})
            string(REGEX REPLACE "^<file[^>]*>" "" RC_FILE "${RC_FILE}")
            if(NOT IS_ABSOLUTE "${RC_FILE}")
                set(RC_FILE "${rc_path}/${RC_FILE}")
            endif()
            set(RC_DEPENDS ${RC_DEPENDS} "${RC_FILE}")
        endforeach()
        # Since this cmake macro is doing the dependency scanning for these files,
        # let's make a configured file and add it as a dependency so cmake is run
        # again when dependencies need to be recomputed.
        _qt_internal_make_output_file("${infile}" "" "qrc.depends" out_depends)
        configure_file("${infile}" "${out_depends}" COPYONLY)
    else()
        # The .qrc file does not exist (yet). Let's add a dependency and hope
        # that it will be generated later
        set(out_depends)
    endif()

    set(${_out_depends} ${out_depends} PARENT_SCOPE)
    set(${_rc_depends} ${RC_DEPENDS} PARENT_SCOPE)
endfunction()


# qt6_add_binary_resources(target inputfiles ... )

function(qt6_add_binary_resources target )

    set(options)
    set(oneValueArgs DESTINATION)
    set(multiValueArgs OPTIONS)

    cmake_parse_arguments(_RCC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(rcc_files ${_RCC_UNPARSED_ARGUMENTS})
    set(rcc_options ${_RCC_OPTIONS})
    set(rcc_destination ${_RCC_DESTINATION})

    if(NOT rcc_destination)
        set(rcc_destination ${CMAKE_CURRENT_BINARY_DIR}/${target}.rcc)
    endif()

    foreach(it ${rcc_files})
        get_filename_component(infile ${it} ABSOLUTE)

        _qt6_parse_qrc_file(${infile} _out_depends _rc_depends)
        set_source_files_properties(${infile} PROPERTIES SKIP_AUTORCC ON)
        set(infiles ${infiles} ${infile})
        set(out_depends ${out_depends} ${_out_depends})
        set(rc_depends ${rc_depends} ${_rc_depends})
    endforeach()

    add_custom_command(OUTPUT ${rcc_destination}
                       DEPENDS ${QT_CMAKE_EXPORT_NAMESPACE}::rcc
                       COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::rcc
                       ARGS ${rcc_options} --binary --name ${target} --output ${rcc_destination} ${infiles}
                       DEPENDS
                            ${QT_CMAKE_EXPORT_NAMESPACE}::rcc
                            ${rc_depends}
                            ${out_depends}
                            ${infiles}
                       VERBATIM)

    add_custom_target(${target} ALL DEPENDS ${rcc_destination})
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_binary_resources)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 5)
            qt5_add_binary_resources(${ARGV})
        elseif(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_add_binary_resources(${ARGV})
        endif()
    endfunction()
endif()


# qt6_add_resources(target resourcename ...
# or
# qt6_add_resources(outfiles inputfile ... )

function(qt6_add_resources outfiles )
    if (TARGET ${outfiles})
        cmake_parse_arguments(arg "" "OUTPUT_TARGETS" "" ${ARGN})
        _qt_internal_process_resource(${ARGV})
        if (arg_OUTPUT_TARGETS)
            set(${arg_OUTPUT_TARGETS} ${${arg_OUTPUT_TARGETS}} PARENT_SCOPE)
        endif()
    else()
        set(options)
        set(oneValueArgs)
        set(multiValueArgs OPTIONS)

        cmake_parse_arguments(_RCC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

        set(rcc_files ${_RCC_UNPARSED_ARGUMENTS})
        set(rcc_options ${_RCC_OPTIONS})

        if("${rcc_options}" MATCHES "-binary")
            message(WARNING "Use qt6_add_binary_resources for binary option")
        endif()

        foreach(it ${rcc_files})
            get_filename_component(outfilename ${it} NAME_WE)
            get_filename_component(infile ${it} ABSOLUTE)
            set(outfile ${CMAKE_CURRENT_BINARY_DIR}/qrc_${outfilename}.cpp)

            _qt6_parse_qrc_file(${infile} _out_depends _rc_depends)
            set_source_files_properties(${infile} PROPERTIES SKIP_AUTORCC ON)

            add_custom_command(OUTPUT ${outfile}
                               COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::rcc
                               ARGS ${rcc_options} --name ${outfilename} --output ${outfile} ${infile}
                               MAIN_DEPENDENCY ${infile}
                               DEPENDS ${_rc_depends} "${_out_depends}" ${QT_CMAKE_EXPORT_NAMESPACE}::rcc
                               VERBATIM)
            set_source_files_properties(${outfile} PROPERTIES SKIP_AUTOMOC ON)
            set_source_files_properties(${outfile} PROPERTIES SKIP_AUTOUIC ON)
            list(APPEND ${outfiles} ${outfile})
        endforeach()
        set(${outfiles} ${${outfiles}} PARENT_SCOPE)
    endif()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_resources outfiles)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 5)
            qt5_add_resources("${outfiles}" ${ARGN})
        elseif(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_add_resources("${outfiles}" ${ARGN})
        endif()
        if(NOT TARGET ${outfiles})
            set("${outfiles}" "${${outfiles}}" PARENT_SCOPE)
        endif()
    endfunction()
endif()


# qt6_add_big_resources(outfiles inputfile ... )

function(qt6_add_big_resources outfiles )
    if (CMAKE_VERSION VERSION_LESS 3.9)
        message(FATAL_ERROR, "qt6_add_big_resources requires CMake 3.9 or newer")
    endif()

    set(options)
    set(oneValueArgs)
    set(multiValueArgs OPTIONS)

    cmake_parse_arguments(_RCC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(rcc_files ${_RCC_UNPARSED_ARGUMENTS})
    set(rcc_options ${_RCC_OPTIONS})

    if("${rcc_options}" MATCHES "-binary")
        message(WARNING "Use qt6_add_binary_resources for binary option")
    endif()

    foreach(it ${rcc_files})
        get_filename_component(outfilename ${it} NAME_WE)
        get_filename_component(infile ${it} ABSOLUTE)
        set(tmpoutfile ${CMAKE_CURRENT_BINARY_DIR}/qrc_${outfilename}tmp.cpp)
        set(outfile ${CMAKE_CURRENT_BINARY_DIR}/qrc_${outfilename}.o)

        _qt6_parse_qrc_file(${infile} _out_depends _rc_depends)
        set_source_files_properties(${infile} PROPERTIES SKIP_AUTORCC ON)
        add_custom_command(OUTPUT ${tmpoutfile}
                           COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::rcc ${rcc_options} --name ${outfilename} --pass 1 --output ${tmpoutfile} ${infile}
                           DEPENDS ${infile} ${_rc_depends} "${out_depends}" ${QT_CMAKE_EXPORT_NAMESPACE}::rcc
                           VERBATIM)
        add_custom_target(big_resources_${outfilename} ALL DEPENDS ${tmpoutfile})
        add_library(rcc_object_${outfilename} OBJECT ${tmpoutfile})
        target_compile_definitions(rcc_object_${outfilename} PUBLIC "$<TARGET_PROPERTY:Qt6::Core,INTERFACE_COMPILE_DEFINITIONS>")
        set_target_properties(rcc_object_${outfilename} PROPERTIES AUTOMOC OFF)
        set_target_properties(rcc_object_${outfilename} PROPERTIES AUTOUIC OFF)
        add_dependencies(rcc_object_${outfilename} big_resources_${outfilename})
        # The modification of TARGET_OBJECTS needs the following change in cmake
        # https://gitlab.kitware.com/cmake/cmake/commit/93c89bc75ceee599ba7c08b8fe1ac5104942054f
        add_custom_command(OUTPUT ${outfile}
                           COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::rcc
                           ARGS ${rcc_options} --name ${outfilename} --pass 2 --temp $<TARGET_OBJECTS:rcc_object_${outfilename}> --output ${outfile} ${infile}
                           DEPENDS rcc_object_${outfilename} $<TARGET_OBJECTS:rcc_object_${outfilename}> ${QT_CMAKE_EXPORT_NAMESPACE}::rcc
                           VERBATIM)
       list(APPEND ${outfiles} ${outfile})
    endforeach()
    set(${outfiles} ${${outfiles}} PARENT_SCOPE)
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_big_resources outfiles)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 5)
            qt5_add_big_resources(${outfiles} ${ARGN})
        elseif(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_add_big_resources(${outfiles} ${ARGN})
        endif()
        set("${outfiles}" "${${outfiles}}" PARENT_SCOPE)
    endfunction()
endif()

function(_qt_internal_apply_win_prefix_and_suffix target)
    if(WIN32)
        # Table of prefix / suffixes for MSVC libraries as qmake expects them to be created.
        # static - Qt6EdidSupport.lib (platform support libraries / or static QtCore, etc)
        # shared - Qt6Core.dll
        # shared import library - Qt6Core.lib
        # module aka Qt plugin - qwindows.dll
        # module import library - qwindows.lib
        #
        # The CMake defaults are fine for us.

        # Table of prefix / suffixes for MinGW libraries as qmake expects them to be created.
        # static - libQt6EdidSupport.a (platform support libraries / or static QtCore, etc)
        # shared - Qt6Core.dll
        # shared import library - libQt6Core.a
        # module aka Qt plugin - qwindows.dll
        # module import library - libqwindows.a
        #
        # CMake for Windows-GNU platforms defaults the prefix to "lib".
        # CMake for Windows-GNU platforms defaults the import suffix to ".dll.a".
        # These CMake defaults are not ok for us.

        # This should cover both MINGW with GCC and CLANG.
        if(NOT MSVC)
            set_property(TARGET "${target}" PROPERTY IMPORT_SUFFIX ".a")

            get_target_property(target_type ${target} TYPE)
            if(target_type STREQUAL "STATIC_LIBRARY")
                set_property(TARGET "${target}" PROPERTY PREFIX "lib")
            else()
                set_property(TARGET "${target}" PROPERTY PREFIX "")
                set_property(TARGET "${target}" PROPERTY IMPORT_PREFIX "lib")
            endif()
        endif()
    endif()
endfunction()

set(_Qt6_COMPONENT_PATH "${CMAKE_CURRENT_LIST_DIR}/..")

# This function is currently in Technical Preview.
# It's signature and behavior might change.
#
# Wrapper function that adds an executable with some Qt specific behavior.
# Some scenarios require steps to be deferred to the end of the current
# directory scope so that the caller has an opportunity to modify certain
# target properties.
function(qt6_add_executable target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "MANUAL_FINALIZATION" "" "")

    _qt_internal_create_executable("${target}" ${arg_UNPARSED_ARGUMENTS})

    if(arg_MANUAL_FINALIZATION)
        # Caller says they will call qt6_finalize_executable() themselves later
        return()
    endif()

    # Defer the finalization if we can. When the caller's project requires
    # CMake 3.19 or later, this makes the calls to this function concise while
    # still allowing target property modification before finalization.
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
        # Need to wrap in an EVAL CODE or else ${target} won't be evaluated
        # due to special behavior of cmake_language() argument handling
        cmake_language(EVAL CODE "cmake_language(DEFER CALL qt6_finalize_executable ${target})")
    else()
        qt6_finalize_executable("${target}")
    endif()
endfunction()

function(_qt_internal_create_executable target)
    if(ANDROID)
        list(REMOVE_ITEM ARGN "WIN32" "MACOSX_BUNDLE")
        add_library("${target}" MODULE ${ARGN})
        # On our qmake builds we do don't compile the executables with
        # visibility=hidden. Not having this flag set will cause the
        # executable to have main() hidden and can then no longer be loaded
        # through dlopen()
        set_property(TARGET "${target}" PROPERTY C_VISIBILITY_PRESET default)
        set_property(TARGET "${target}" PROPERTY CXX_VISIBILITY_PRESET default)
        set_property(TARGET "${target}" PROPERTY OBJC_VISIBILITY_PRESET default)
        set_property(TARGET "${target}" PROPERTY OBJCXX_VISIBILITY_PRESET default)
        qt6_android_apply_arch_suffix("${target}")
    else()
        add_executable("${target}" ${ARGN})
    endif()

    target_link_libraries("${target}" PRIVATE Qt6::Core)
endfunction()

# This function is currently in Technical Preview.
# It's signature and behavior might change.
function(qt6_finalize_executable target)

    # We can't evaluate generator expressions at configure time, so we can't
    # ask for any transitive properties or even the full library dependency
    # chain. We can still look at the immediate dependencies though and query
    # any that are not expressed as generator expressions. For any we can
    # identify as a CMake target known to the current scope, we can check if
    # that target has a finalizer to be called. This is expected to cover the
    # vast majority of use cases, since projects will typically link directly
    # to Qt::* targets. For the cases where this isn't so, the project will be
    # responsible for calling any relevant functions themselves instead of
    # relying on these automatic finalization calls.
    set(finalizers)
    get_target_property(immediate_deps ${target} LINK_LIBRARIES)
    if(immediate_deps)
        foreach(dep IN LISTS immediate_deps)
            if(NOT TARGET ${dep})
                continue()
            endif()
            get_target_property(dep_finalizers ${dep}
                INTERFACE_QT_EXECUTABLE_FINALIZERS
            )
            if(dep_finalizers)
                list(APPEND finalizers ${dep_finalizers})
            endif()
        endforeach()
        list(REMOVE_DUPLICATES finalizers)
    endif()
    if(finalizers)
        if(CMAKE_VERSION VERSION_LESS 3.18)
            # cmake_language() not available
            message(WARNING
                "Skipping module-specific finalizers for target ${target} "
                "(requires CMake 3.18 or later)"
            )
        else()
            foreach(finalizer_func IN LISTS finalizers)
                cmake_language(CALL ${finalizer_func} ${target})
            endforeach()
        endif()
    endif()

    if(ANDROID)
        qt6_android_generate_deployment_settings("${target}")
        qt6_android_add_apk_target("${target}")
    endif()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_executable)
        qt6_add_executable(${ARGV})
    endfunction()
    function(qt_finalize_executable)
        qt6_finalize_executable(${ARGV})
    endfunction()
endif()

# Temporarily keep compatibility, until all repositories are migrated.
function(add_qt_gui_executable)
    message(AUTHOR_WARNING "Please replace add_qt_gui_executable with qt_add_executable instead. The former will be removed shortly.")
    qt6_add_executable(${ARGV})
endfunction()

function(_qt_get_plugin_name_with_version target out_var)
    string(REGEX REPLACE "^Qt::(.+)" "Qt${QT_DEFAULT_MAJOR_VERSION}::\\1"
           qt_plugin_with_version "${target}")
    if(TARGET "${qt_plugin_with_version}")
        set("${out_var}" "${qt_plugin_with_version}" PARENT_SCOPE)
    else()
        set("${out_var}" "" PARENT_SCOPE)
    endif()
endfunction()

macro(_qt_import_plugin target plugin)
    set(_final_plugin_name "${plugin}")
    if(NOT TARGET "${plugin}")
        _qt_get_plugin_name_with_version("${plugin}" _qt_plugin_with_version_name)
        if(TARGET "${_qt_plugin_with_version_name}")
            set(_final_plugin_name "${_qt_plugin_with_version_name}")
        endif()
    endif()

    if(NOT TARGET "${_final_plugin_name}")
        message(
            "Warning: plug-in ${_final_plugin_name} is not known to the current Qt installation.")
    else()
        get_target_property(_plugin_class_name "${_final_plugin_name}" QT_PLUGIN_CLASS_NAME)
        if(_plugin_class_name)
            set_property(TARGET "${target}" APPEND PROPERTY QT_PLUGINS "${plugin}")
        endif()
    endif()
endmacro()

function(_qt_internal_disable_static_default_plugins target)
    set_target_properties(${target} PROPERTIES QT_DEFAULT_PLUGINS 0)
endfunction()

# This function is used to indicate which plug-ins are going to be
# used by a given target.
# This allows static linking to a correct set of plugins.
# Options :
#    NO_DEFAULT: disable linking against any plug-in by default for that target, e.g. no platform plug-in.
#    INCLUDE <list of additional plug-ins to be linked against>
#    EXCLUDE <list of plug-ins to be removed from the default set>
#    INCLUDE_BY_TYPE <type> <included plugins>
#    EXCLUDE_BY_TYPE <type to be excluded>
#
# Example :
# qt_import_plugins(myapp
#     INCLUDE Qt::QCocoaIntegrationPlugin
#     EXCLUDE Qt::QMinimalIntegrationPlugin
#     INCLUDE_BY_TYPE imageformats Qt::QGifPlugin Qt::QJpegPlugin
#     EXCLUDE_BY_TYPE sqldrivers
# )

# TODO : support qml plug-ins.
function(qt6_import_plugins target)
    cmake_parse_arguments(arg "NO_DEFAULT" "" "INCLUDE;EXCLUDE;INCLUDE_BY_TYPE;EXCLUDE_BY_TYPE" ${ARGN})

    # Handle NO_DEFAULT
    if(${arg_NO_DEFAULT})
        _qt_internal_disable_static_default_plugins("${target}")
    endif()

    # Handle INCLUDE
    foreach(plugin ${arg_INCLUDE})
        _qt_import_plugin("${target}" "${plugin}")
    endforeach()

    # Handle EXCLUDE
    foreach(plugin ${arg_EXCLUDE})
        set_property(TARGET "${target}" APPEND PROPERTY QT_NO_PLUGINS "${plugin}")
    endforeach()

    # Handle INCLUDE_BY_TYPE
    set(_current_type "")
    foreach(_arg ${arg_INCLUDE_BY_TYPE})
        string(REGEX REPLACE "[-/]" "_" _plugin_type "${_arg}")
        list(FIND QT_ALL_PLUGIN_TYPES_FOUND_VIA_FIND_PACKAGE "${_plugin_type}" _has_plugin_type)

        if(${_has_plugin_type} GREATER_EQUAL 0)
           set(_current_type "${_plugin_type}")
        else()
            if("${_current_type}" STREQUAL "")
                message(FATAL_ERROR "qt_import_plugins: invalid syntax for INCLUDE_BY_TYPE")
            endif()

            # Check if passed plugin target name is a version-less one, and make a version-full
            # one.
            _qt_get_plugin_name_with_version("${_arg}" qt_plugin_with_version)
            if(TARGET "${_arg}" OR TARGET "${qt_plugin_with_version}")
                set_property(TARGET "${target}" APPEND PROPERTY "QT_PLUGINS_${_current_type}" "${_arg}")
            else()
                message("Warning: plug-in ${_arg} is not known to the current Qt installation.")
            endif()
        endif()
    endforeach()

    # Handle EXCLUDE_BY_TYPE
    foreach(_arg ${arg_EXCLUDE_BY_TYPE})
        string(REGEX REPLACE "[-/]" "_" _plugin_type "${_arg}")
        set_property(TARGET "${target}" PROPERTY "QT_PLUGINS_${_plugin_type}" "-")
    endforeach()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_import_plugins)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 5)
            qt5_import_plugins(${ARGV})
        elseif(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_import_plugins(${ARGV})
        endif()
    endfunction()
endif()


# Extracts metatypes from a Qt target and generates a metatypes.json for it.
# By default we check whether AUTOMOC has been enabled and we extract the information from the
# target's AUTOMOC supporting files.
# Should you not wish to use automoc you need to pass in all the generated json files via the
# MANUAL_MOC_JSON_FILES parameter. The latter can be obtained by running moc with
# the --output-json parameter.
# Params:
#   INSTALL_DIR: Location where to install the metatypes file. For public consumption,
#                defaults to a ${CMAKE_INSTALL_PREFIX}/${INSTALL_LIBDIR}/metatypes directory.
#                Executable metatypes files are never installed.
#   COPY_OVER_INSTALL: (Qt Internal) When present will install the file via a post build step
#   copy rather than using install.
function(qt6_extract_metatypes target)

    get_target_property(existing_meta_types_file ${target} INTERFACE_QT_META_TYPES_BUILD_FILE)
    if (existing_meta_types_file)
        return()
    endif()

    cmake_parse_arguments(arg "COPY_OVER_INSTALL" "INSTALL_DIR" "MANUAL_MOC_JSON_FILES" ${ARGN})

    get_target_property(target_type ${target} TYPE)
    if (target_type STREQUAL "INTERFACE_LIBRARY")
        message(FATAL_ERROR "Meta types generation does not work on interface libraries")
        return()
    endif()

    if (CMAKE_VERSION VERSION_LESS "3.16.0")
        message(FATAL_ERROR "Meta types generation requires CMake >= 3.16")
        return()
    endif()

    # Whether the generated json file needs to be installed for prefix-builds, or copied for
    # non-prefix builds. Regardless of the type of build, executable metatypes.json files should
    # not be installed. Only library .json files should be installed.
    set(should_install "TRUE")
    if (target_type STREQUAL "EXECUTABLE")
        set(should_install "FALSE")
    endif()

    # Automatically fill default install args when not specified.
    if (NOT arg_INSTALL_DIR)
        # INSTALL_LIBDIR is not set when QtBuildInternals is not loaded (when not doing a Qt build).
        if(INSTALL_LIBDIR)
            set(arg_INSTALL_DIR "${INSTALL_LIBDIR}/metatypes")
        else()
            set(arg_INSTALL_DIR "lib/metatypes")
        endif()
    endif()

    get_target_property(target_binary_dir ${target} BINARY_DIR)
    set(type_list_file "${target_binary_dir}/meta_types/${target}_json_file_list.txt")
    set(type_list_file_manual "${target_binary_dir}/meta_types/${target}_json_file_list_manual.txt")

    get_target_property(uses_automoc ${target} AUTOMOC)
    set(automoc_args)
    set(automoc_dependencies)
    # Handle automoc generated data
    if (uses_automoc)
        # Tell automoc to output json files)
        set_property(TARGET "${target}" APPEND PROPERTY
            AUTOMOC_MOC_OPTIONS "--output-json"
        )

        if(NOT CMAKE_CONFIGURATION_TYPES)
            set(cmake_autogen_cache_file
                "${target_binary_dir}/CMakeFiles/${target}_autogen.dir/ParseCache.txt")
            set(mutli_config_args
                --cmake-autogen-include-dir-path "${target_binary_dir}/${target}_autogen/include"
            )
        else()
            set(cmake_autogen_cache_file
                "${target_binary_dir}/CMakeFiles/${target}_autogen.dir/ParseCache_$<CONFIG>.txt")
            set(mutli_config_args
                --cmake-autogen-include-dir-path "${target_binary_dir}/${target}_autogen/include_$<CONFIG>"
                "--cmake-multi-config")
        endif()

        set(cmake_autogen_info_file
            "${target_binary_dir}/CMakeFiles/${target}_autogen.dir/AutogenInfo.json")

        set (use_dep_files FALSE)
        if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.17") # Requires automoc changes present only in 3.17
            if(CMAKE_GENERATOR STREQUAL "Ninja" OR CMAKE_GENERATOR STREQUAL "Ninja Multi-Config")
                set(use_dep_files TRUE)
            endif()
        endif()

        if (NOT use_dep_files)
            add_custom_target(${target}_automoc_json_extraction
                DEPENDS ${QT_CMAKE_EXPORT_NAMESPACE}::cmake_automoc_parser
                BYPRODUCTS ${type_list_file}
                COMMAND
                    ${QT_CMAKE_EXPORT_NAMESPACE}::cmake_automoc_parser
                    --cmake-autogen-cache-file "${cmake_autogen_cache_file}"
                    --cmake-autogen-info-file "${cmake_autogen_info_file}"
                    --output-file-path "${type_list_file}"
                    ${mutli_config_args}
                COMMENT "Running Automoc file extraction"
                COMMAND_EXPAND_LISTS
            )
            add_dependencies(${target}_automoc_json_extraction ${target}_autogen)
        else()
            set(cmake_autogen_timestamp_file
                "${target_binary_dir}/${target}_autogen/timestamp"
            )

            add_custom_command(OUTPUT ${type_list_file}
                DEPENDS ${QT_CMAKE_EXPORT_NAMESPACE}::cmake_automoc_parser
                    ${cmake_autogen_timestamp_file}
                COMMAND
                    ${QT_CMAKE_EXPORT_NAMESPACE}::cmake_automoc_parser
                    --cmake-autogen-cache-file "${cmake_autogen_cache_file}"
                    --cmake-autogen-info-file "${cmake_autogen_info_file}"
                    --output-file-path "${type_list_file}"
                    ${mutli_config_args}
                COMMENT "Running Automoc file extraction"
                COMMAND_EXPAND_LISTS
            )

        endif()
        set(automoc_args "@${type_list_file}")
        set(automoc_dependencies "${type_list_file}")
    endif()

    set(manual_args)
    set(manual_dependencies)
    if(arg_MANUAL_MOC_JSON_FILES)
        list(REMOVE_DUPLICATES arg_MANUAL_MOC_JSON_FILES)
        file(GENERATE
            OUTPUT ${type_list_file_manual}
            CONTENT "$<JOIN:$<GENEX_EVAL:${arg_MANUAL_MOC_JSON_FILES}>,\n>"
        )
        list(APPEND manual_dependencies ${arg_MANUAL_MOC_JSON_FILES} ${type_list_file_manual})
        set(manual_args "@${type_list_file_manual}")
    endif()

    if (NOT manual_args AND NOT automoc_args)
        message(FATAL_ERROR "Metatype generation requires either the use of AUTOMOC or a manual list of generated json files")
    endif()

    if (CMAKE_BUILD_TYPE)
        string(TOLOWER ${target}_${CMAKE_BUILD_TYPE} target_lowercase)
    else()
        string(TOLOWER ${target} target_lowercase)
    endif()

    set(metatypes_file_name "qt6${target_lowercase}_metatypes.json")
    set(metatypes_file "${target_binary_dir}/meta_types/${metatypes_file_name}")
    set(metatypes_file_gen "${target_binary_dir}/meta_types/${metatypes_file_name}.gen")

    set(metatypes_dep_file_name "qt6${target_lowercase}_metatypes_dep.txt")
    set(metatypes_dep_file "${target_binary_dir}/meta_types/${metatypes_dep_file_name}")

    # Due to generated source file dependency rules being tied to the directory
    # scope in which they are created it is not possible for other targets which
    # are defined in a separate scope to see these rules. This leads to failures
    # in locating the generated source files.
    # To work around this we write a dummy file to disk to make sure targets
    # which link against the current target do not produce the error. This dummy
    # file is then replaced with the contents of the generated file during
    # build.
    if (NOT EXISTS ${metatypes_file})
        file(MAKE_DIRECTORY "${target_binary_dir}/meta_types")
        file(TOUCH ${metatypes_file})
    endif()

    # Need to make the path absolute during a Qt non-prefix build, otherwise files are written
    # to the source dir because the paths are relative to the source dir when using file(TOUCH).
    if(arg_COPY_OVER_INSTALL AND NOT IS_ABSOLUTE "${arg_INSTALL_DIR}/${metatypes_file_name}")
        set(arg_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${arg_INSTALL_DIR}")
    endif()

    if (should_install AND arg_COPY_OVER_INSTALL
          AND NOT EXISTS ${arg_INSTALL_DIR}/${metatypes_file_name})
        file(MAKE_DIRECTORY "${arg_INSTALL_DIR}")
        file(TOUCH "${arg_INSTALL_DIR}/${metatypes_file_name}")
    endif()
    add_custom_command(OUTPUT ${metatypes_file_gen} ${metatypes_file}
        DEPENDS ${QT_CMAKE_EXPORT_NAMESPACE}::moc ${automoc_dependencies} ${manual_dependencies}
        COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::moc
            -o ${metatypes_file_gen}
            --collect-json ${automoc_args} ${manual_args}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${metatypes_file_gen}
            ${metatypes_file}
        COMMENT "Running automoc with --collect-json"
    )

    # We still need to add this file as a source of Core, otherwise the file
    # rule above is not triggered. INTERFACE_SOURCES do not properly register
    # as dependencies to build the current target.
    target_sources(${target} PRIVATE ${metatypes_file_gen})
    set(metatypes_file_genex_build)
    set(metatypes_file_genex_install)
    if (arg_COPY_OVER_INSTALL)
        if(should_install)
            set(metatypes_file_genex_build
                "$<BUILD_INTERFACE:$<$<BOOL:$<TARGET_PROPERTY:QT_CONSUMES_METATYPES>>:${arg_INSTALL_DIR}/${metatypes_file_name}>>"
            )
        endif()
    else()
        set(metatypes_file_genex_build
            "$<BUILD_INTERFACE:$<$<BOOL:$<TARGET_PROPERTY:QT_CONSUMES_METATYPES>>:${metatypes_file}>>"
        )
        if(should_install)
            set(metatypes_file_genex_install
                "$<INSTALL_INTERFACE:$<$<BOOL:$<TARGET_PROPERTY:QT_CONSUMES_METATYPES>>:$<INSTALL_PREFIX>/${arg_INSTALL_DIR}/${metatypes_file_name}>>"
            )
        endif()
    endif()
    set_source_files_properties(${metatypes_file} PROPERTIES HEADER_FILE_ONLY TRUE)

    set_target_properties(${target} PROPERTIES
        INTERFACE_QT_MODULE_HAS_META_TYPES YES
        INTERFACE_QT_MODULE_META_TYPES_FROM_BUILD YES
        INTERFACE_QT_META_TYPES_BUILD_FILE "${metatypes_file}"
        QT_MODULE_META_TYPES_FILE_GENEX_BUILD "${metatypes_file_genex_build}"
        QT_MODULE_META_TYPES_FILE_GENEX_INSTALL "${metatypes_file_genex_install}"
    )
    target_sources(${target} INTERFACE ${metatypes_file_genex_build} ${metatypes_file_genex_install})

    # Installation is complicated, because there are multiple combinations.
    # In non-prefix builds (signaled by arg_COPY_OVER_INSTALL == TRUE), Qt modules are /copied/
    # into the qt_prefix/lib/metatypes.
    # In prefix builds (signaled by arg_COPY_OVER_INSTALL == FALSE), Qt modules are /installed/
    # into the qt_prefix/lib/metatypes.
    # Currently only the internal qt_add_module sets arg_COPY_OVER_INSTALL.
    #
    # Tests and examples are executables, and thus will not have their meta types installed, but
    # they will have them generated (if this function is called).
    #
    # Regular libraries and plugins (which are not part of the Qt build), will be /installed/
    # into a lib/metatypes directory relative to their prefix, rather than the Qt prefix (only
    # outside of a Qt build).
    # We don't support non-prefix builds for libraries or plugins which are not part of the official
    # Qt build. Aka everything non-prefix / COPY_OVER_INSTALL related are implementation details
    # that users shouldn't use.
    if(should_install)
        if (arg_COPY_OVER_INSTALL)
            set(command_args
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${metatypes_file}"
                    "${arg_INSTALL_DIR}/${metatypes_file_name}"
            )
            if (target_type STREQUAL "OBJECT_LIBRARY")
                add_custom_target(${target}_metatypes_copy
                    DEPENDS "${metatypes_file}"
                    ${command_args}
                )
                add_dependencies(${target} ${target}_metatypes_copy)
            else()
                add_custom_command(TARGET ${target} POST_BUILD
                    ${command_args}
                )
            endif()
        else()
            install(FILES "${metatypes_file}" DESTINATION "${arg_INSTALL_DIR}")
        endif()
    endif()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_extract_metatypes)
        qt6_extract_metatypes(${ARGV})
    endfunction()
endif()

# Generate Win32 RC files for a target. All entries in the RC file are generated
# from target properties:
#
# QT_TARGET_COMPANY_NAME: RC Company name
# QT_TARGET_DESCRIPTION: RC File Description
# QT_TARGET_VERSION: RC File and Product Version
# QT_TARGET_COPYRIGHT: RC LegalCopyright
# QT_TARGET_PRODUCT_NAME: RC ProductName
# QT_TARGET_COMMENTS: RC Comments
# QT_TARGET_ORIGINAL_FILENAME: RC Original FileName
# QT_TARGET_TRADEMARKS: RC LegalTrademarks
# QT_TARGET_INTERNALNAME: RC InternalName
# QT_TARGET_RC_ICONS: List of paths to icon files
#
# If you do not wish to auto-generate rc files, it's possible to provide your
# own RC file by setting the property QT_TARGET_WINDOWS_RC_FILE with a path to
# an existing rc file.
function(_qt_internal_generate_win32_rc_file target)
    set(prohibited_target_types INTERFACE_LIBRARY STATIC_LIBRARY OBJECT_LIBRARY)
    get_target_property(target_type ${target} TYPE)
    if(target_type IN_LIST prohibited_target_types)
        return()
    endif()

    get_target_property(target_binary_dir ${target} BINARY_DIR)

    get_target_property(target_rc_file ${target} QT_TARGET_WINDOWS_RC_FILE)
    get_target_property(target_version ${target} QT_TARGET_VERSION)

    if (NOT target_rc_file AND NOT target_version)
        return()
    endif()

    if (target_rc_file)
        # Use the provided RC file
        target_sources(${target} PRIVATE "${target_rc_file}")
    else()
        # Generate RC File
        set(rc_file_output "${target_binary_dir}/")
        if(QT_GENERATOR_IS_MULTI_CONFIG)
            string(APPEND rc_file_output "$<CONFIG>/")
        endif()
        string(APPEND rc_file_output "${target}_resource.rc")
        set(target_rc_file "${rc_file_output}")

        set(company_name "")
        get_target_property(target_company_name ${target} QT_TARGET_COMPANY_NAME)
        if (target_company_name)
            set(company_name "${target_company_name}")
        endif()

        set(file_description "")
        get_target_property(target_description ${target} QT_TARGET_DESCRIPTION)
        if (target_description)
            set(file_description "${target_description}")
        endif()

        set(legal_copyright "")
        get_target_property(target_copyright ${target} QT_TARGET_COPYRIGHT)
        if (target_copyright)
            set(legal_copyright "${target_copyright}")
        endif()

        set(product_name "")
        get_target_property(target_product_name ${target} QT_TARGET_PRODUCT_NAME)
        if (target_product_name)
            set(product_name "${target_product_name}")
        else()
            set(product_name "${target}")
        endif()

        set(comments "")
        get_target_property(target_comments ${target} QT_TARGET_COMMENTS)
        if (target_comments)
            set(comments "${target_comments}")
        endif()

        set(legal_trademarks "")
        get_target_property(target_trademarks ${target} QT_TARGET_TRADEMARKS)
        if (target_trademarks)
            set(legal_trademarks "${target_trademarks}")
        endif()

        set(product_version "")
        if (target_version)
            if(target_version MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+")
                # nothing to do
            elseif(target_version MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+")
                set(target_version "${target_version}.0")
            elseif(target_version MATCHES "[0-9]+\\.[0-9]+")
                set(target_version "${target_version}.0.0")
            elseif (target_version MATCHES "[0-9]+")
                set(target_version "${target_version}.0.0.0")
            else()
                message(FATAL_ERROR "Invalid version format: '${target_version}'")
            endif()
            set(product_version "${target_version}")
        else()
            set(product_version "0.0.0.0")
        endif()

        set(file_version "${product_version}")
        string(REPLACE "." "," version_comma ${product_version})

        set(original_file_name "$<TARGET_FILE_NAME:${target}>")
        get_target_property(target_original_file_name ${target} QT_TARGET_ORIGINAL_FILENAME)
        if (target_original_file_name)
            set(original_file_name "${target_original_file_name}")
        endif()

        set(internal_name "")
        get_target_property(target_internal_name ${target} QT_TARGET_INTERNALNAME)
        if (target_internal_name)
            set(internal_name "${target_internal_name}")
        endif()

        set(icons "")
        get_target_property(target_icons ${target} QT_TARGET_RC_ICONS)
        if (target_icons)
            set(index 1)
            foreach( icon IN LISTS target_icons)
                string(APPEND icons "IDI_ICON${index}    ICON    \"${icon}\"\n")
                math(EXPR index "${index} +1")
            endforeach()
        endif()

        set(target_file_type "VFT_DLL")
        if(target_type STREQUAL "EXECUTABLE")
            set(target_file_type "VFT_APP")
        endif()

        set(contents "#include <windows.h>
${icons}
VS_VERSION_INFO VERSIONINFO
FILEVERSION ${version_comma}
PRODUCTVERSION ${version_comma}
FILEFLAGSMASK 0x3fL
#ifdef _DEBUG
    FILEFLAGS VS_FF_DEBUG
#else
    FILEFLAGS 0x0L
#endif
FILEOS VOS_NT_WINDOWS32
FILETYPE ${target_file_type}
FILESUBTYPE VFT2_UNKNOWN
BEGIN
    BLOCK \"StringFileInfo\"
    BEGIN
        BLOCK \"040904b0\"
        BEGIN
            VALUE \"CompanyName\", \"${company_name}\"
            VALUE \"FileDescription\", \"${file_description}\"
            VALUE \"FileVersion\", \"${file_version}\"
            VALUE \"LegalCopyright\", \"${legal_copyright}\"
            VALUE \"OriginalFilename\", \"${original_file_name}\"
            VALUE \"ProductName\", \"${product_name}\"
            VALUE \"ProductVersion\", \"${product_version}\"
            VALUE \"Comments\", \"${comments}\"
            VALUE \"LegalTrademarks\", \"${legal_trademarks}\"
            VALUE \"InternalName\", \"${internal_name}\"
        END
    END
    BLOCK \"VarFileInfo\"
    BEGIN
        VALUE \"Translation\", 0x0409, 1200
    END
END
/* End of Version info */\n"
        )

        # We can't use the output of file generate as source so we work around
        # this by generating the file under a different name and then copying
        # the file in place using add custom command.
        file(GENERATE OUTPUT "${rc_file_output}.tmp"
            CONTENT "${contents}"
        )

        if(QT_GENERATOR_IS_MULTI_CONFIG)
            set(cfgs ${CMAKE_CONFIGURATION_TYPES})
            set(outputs "")
            foreach(cfg ${cfgs})
                string(REPLACE "$<CONFIG>" "${cfg}" expanded_rc_file_output "${rc_file_output}")
                list(APPEND outputs "${expanded_rc_file_output}")
            endforeach()
        else()
            set(cfgs "${CMAKE_BUILD_TYPE}")
            set(outputs "${rc_file_output}")
        endif()

        # We would like to do the following:
        #     target_sources(${target} PRIVATE "$<$<CONFIG:${cfg}>:${output}>")
        # However, https://gitlab.kitware.com/cmake/cmake/-/issues/20682 doesn't let us.
        # Work-around by compiling the resources in an object lib and linking that.
        add_library(${target}_rc OBJECT "${output}")
        target_link_libraries(${target} PRIVATE $<TARGET_OBJECTS:${target}_rc>)
        while(outputs)
            list(POP_FRONT cfgs cfg)
            list(POP_FRONT outputs output)
            set(input "${output}.tmp")
            add_custom_command(OUTPUT "${output}"
                DEPENDS "${input}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${input}" "${output}"
            )
            target_sources(${target}_rc PRIVATE "$<$<CONFIG:${cfg}>:${output}>")
        endwhile()
    endif()
endfunction()

function(__qt_get_relative_resource_path_for_file output_alias file)
    get_property(alias SOURCE ${file} PROPERTY QT_RESOURCE_ALIAS)
    if (NOT alias)
        set(alias "${file}")
    endif()
    set(${output_alias} ${alias} PARENT_SCOPE)
endfunction()

function(__qt_propagate_generated_resource target resource_name generated_source_code output_generated_target)
    get_target_property(type ${target} TYPE)
    if(type STREQUAL STATIC_LIBRARY)
        get_target_property(resource_count ${target} _qt_generated_resource_target_count)
        if(NOT resource_count)
            set(resource_count "0")
        endif()
        math(EXPR resource_count "${resource_count} + 1")
        set_target_properties(${target} PROPERTIES _qt_generated_resource_target_count ${resource_count})

        set(resource_target "${target}_resources_${resource_count}")
        add_library("${resource_target}" OBJECT "${generated_source_code}")
        set_property(TARGET ${resource_target} APPEND PROPERTY _qt_resource_name ${resource_name})

        # Save the path to the generated source file, relative to the the current build dir.
        # The path will be used in static library prl file generation to ensure qmake links
        # against the installed resource object files.
        # Example saved path:
        #    .rcc/qrc_qprintdialog.cpp
        file(RELATIVE_PATH generated_cpp_file_relative_path
            "${CMAKE_CURRENT_BINARY_DIR}"
            "${generated_source_code}")
        set_property(TARGET ${resource_target} APPEND PROPERTY
            _qt_resource_generated_cpp_relative_path "${generated_cpp_file_relative_path}")

        # Use TARGET_NAME genex to map to the correct prefixed target name when it is exported
        # via qt_install(EXPORT), so that the consumers of the target can find the object library
        # as well.
        target_link_libraries(${target} INTERFACE
                              "$<TARGET_OBJECTS:$<TARGET_NAME:${resource_target}>>")
        set(${output_generated_target} "${resource_target}" PARENT_SCOPE)

        # No need to compile Q_IMPORT_PLUGIN-containing files for non-executables.
        _qt_internal_disable_static_default_plugins("${resource_target}")
    else()
        set(${output_generated_target} "" PARENT_SCOPE)
        target_sources(${target} PRIVATE ${generated_source_code})
    endif()
endfunction()

# Creates fake targets and adds resource files to IDE's tree
function(_qt_internal_expose_source_file_to_ide target file)
    set(ide_target_extension "other_files")
    set(qml_extensions ".qml" ".js")
    get_filename_component(resource_extension "${file}" LAST_EXT)
    if(resource_extension IN_LIST qml_extensions)
        set(ide_target_extension "qml_files")
    endif()

    set(ide_target ${target}_${ide_target_extension})
    if(NOT TARGET ${ide_target})
        add_custom_target(${ide_target} SOURCES "${file}")
    else()
        set_property(TARGET ${ide_target} APPEND PROPERTY SOURCES "${file}")
    endif()
endfunction()

#
# Process resources via file path instead of QRC files. Behind the
# scnenes, it will generate a qrc file and apply post processing steps
# when applicable. (e.g.: QtQuickCompiler)
#
# The QRC Prefix is set via the PREFIX parameter.
#
# Alias settings for files need to be set via the QT_RESOURCE_ALIAS property
# via the set_soure_files_properties() command.
#
# When using this command with static libraries, one or more special targets
# will be generated. Should you wish to perform additional processing on these
# targets pass a value to the OUTPUT_TARGETS parameter.
#
function(_qt_internal_process_resource target resourceName)

    cmake_parse_arguments(rcc "" "PREFIX;LANG;BASE;OUTPUT_TARGETS;DESTINATION" "FILES;OPTIONS" ${ARGN})

    if("${rcc_OPTIONS}" MATCHES "-binary")
        set(isBinary TRUE)
    endif()

    string(REPLACE "/" "_" resourceName ${resourceName})
    string(REPLACE "." "_" resourceName ${resourceName})

    set(output_targets "")
    # Apply base to all files
    if (rcc_BASE)
        foreach(file IN LISTS rcc_FILES)
            set(resource_file "${rcc_BASE}/${file}")
            __qt_get_relative_resource_path_for_file(alias ${resource_file})
            # Handle case where resources were generated from a directory
            # different than the one where the main .pro file resides.
            # Unless otherwise specified, we should use the original file path
            # as alias.
            if (alias STREQUAL resource_file)
                set_source_files_properties(${resource_file} PROPERTIES QT_RESOURCE_ALIAS ${file})
            endif()
            file(TO_CMAKE_PATH ${resource_file} resource_file)
            list(APPEND resource_files ${resource_file})
        endforeach()
    else()
        set(resource_files ${rcc_FILES})
    endif()

    if(NOT rcc_PREFIX)
        get_target_property(rcc_PREFIX ${target} QT_RESOURCE_PREFIX)
        if (NOT rcc_PREFIX)
            message(FATAL_ERROR "_qt_internal_process_resource() was called without a PREFIX and the target does not provide QT_RESOURCE_PREFIX. Please either add a PREFIX or make the target ${target} provide a default.")
        endif()
    endif()

    # Apply quick compiler pass. This is only enabled when Qt6QmlMacros is
    # parsed.
    if (QT6_ADD_RESOURCE_DECLARATIVE_EXTENSIONS)
        _qt_internal_quick_compiler_process_resources(${target} ${resourceName}
            FILES ${resource_files}
            PREFIX ${rcc_PREFIX}
            OUTPUT_REMAINING_RESOURCES resources
            OUTPUT_RESOURCE_NAME newResourceName
            OUTPUT_GENERATED_TARGET output_target_quick
        )
    else()
        set(newResourceName ${resourceName})
        set(resources ${resource_files})
    endif()

    if (NOT resources)
        if (rcc_OUTPUT_TARGETS)
            set(${rcc_OUTPUT_TARGETS} "${output_target_quick}" PARENT_SCOPE)
        endif()
        return()
    endif()
    list(APPEND output_targets ${output_target_quick})
    set(generatedBaseName "${newResourceName}")
    set(generatedResourceFile "${CMAKE_CURRENT_BINARY_DIR}/.rcc/${generatedBaseName}.qrc")

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

    set(resource_dependencies)
    foreach(file IN LISTS resources)
        __qt_get_relative_resource_path_for_file(file_resource_path ${file})

        if (NOT IS_ABSOLUTE ${file})
            set(file "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
        endif()

        ### FIXME: escape file paths to be XML conform
        # <file ...>...</file>
        string(APPEND qrcContents "    <file alias=\"${file_resource_path}\">")
        string(APPEND qrcContents "${file}</file>\n")
        list(APPEND files "${file}")

        get_source_file_property(target_dependency ${file} QT_RESOURCE_TARGET_DEPENDENCY)
        if (NOT target_dependency)
            list(APPEND resource_dependencies ${file})
        else()
            if (NOT TARGET ${target_dependency})
                message(FATAL_ERROR "Target dependency on resource file ${file} is not a cmake target.")
            endif()
            list(APPEND resource_dependencies ${target_dependency})
        endif()
        _qt_internal_expose_source_file_to_ide(${target} "${file}")
    endforeach()

    # </qresource></RCC>
    string(APPEND qrcContents "  </qresource>\n</RCC>\n")

    set(template_file "${__qt_core_macros_module_base_dir}/Qt6CoreConfigureFileTemplate.in")
    set(qt_core_configure_file_contents "${qrcContents}")
    configure_file("${template_file}" "${generatedResourceFile}")

    set(rccArgs --name "${newResourceName}" "${generatedResourceFile}")

    if(rcc_OPTIONS)
        list(APPEND rccArgs ${rcc_OPTIONS})
    endif()

    # When cross-building, we use host tools to generate target code. If the host rcc was compiled
    # with zstd support, it expects the target QtCore to be able to decompress zstd compressed
    # content. This might be true with qmake where host tools are built as part of the
    # cross-compiled Qt, but with CMake we build tools separate from the cross-compiled Qt.
    # If the target does not support zstd (feature is disabled), tell rcc not to generate
    # zstd related code.
    if(NOT QT_FEATURE_zstd)
        list(APPEND rccArgs "--no-zstd")
    endif()

    set_property(SOURCE "${generatedResourceFile}" PROPERTY SKIP_AUTOGEN ON)

    # Set output file name for rcc command
    if(isBinary)
        set(generatedOutfile "${CMAKE_CURRENT_BINARY_DIR}/${generatedBaseName}.rcc")
        if(rcc_DESTINATION)
            # Add .rcc suffix if it's not specified by user
            get_filename_component(destinationRccExt "${rcc_DESTINATION}" LAST_EXT)
            if("${destinationRccExt}" STREQUAL ".rcc")
                set(generatedOutfile "${rcc_DESTINATION}")
            else()
                set(generatedOutfile "${rcc_DESTINATION}.rcc")
            endif()
        endif()
    else()
        set(generatedOutfile "${CMAKE_CURRENT_BINARY_DIR}/.rcc/qrc_${newResourceName}.cpp")
    endif()

    list(PREPEND rccArgs --output "${generatedOutfile}")

    # Process .qrc file:
    add_custom_command(OUTPUT "${generatedOutfile}"
                       COMMAND "${QT_CMAKE_EXPORT_NAMESPACE}::rcc" ${rccArgs}
                       DEPENDS
                        ${resource_dependencies}
                        ${generatedResourceFile}
                        "${QT_CMAKE_EXPORT_NAMESPACE}::rcc"
                       COMMENT "RCC ${newResourceName}"
                       VERBATIM)

    if(isBinary)
        # Add generated .rcc target to 'all' set
        add_custom_target(binary_resource_${generatedBaseName} ALL DEPENDS "${generatedOutfile}")
    else()
        set_property(SOURCE "${generatedOutfile}" PROPERTY SKIP_AUTOGEN ON)
        set_property(TARGET ${target} APPEND PROPERTY _qt_generated_qrc_files "${generatedResourceFile}")

        # Only do this if newResourceName is the same as resourceName, since
        # the resource will be chainloaded by the qt quickcompiler
        # qml cache loader
        if(newResourceName STREQUAL resourceName)
            __qt_propagate_generated_resource(${target} ${resourceName} "${generatedOutfile}" output_target)
            list(APPEND output_targets ${output_target})
        else()
            target_sources(${target} PRIVATE "${generatedOutfile}")
        endif()
        if (rcc_OUTPUT_TARGETS)
            set(${rcc_OUTPUT_TARGETS} "${output_targets}" PARENT_SCOPE)
        endif()
    endif()
endfunction()

macro(_qt_internal_get_add_plugin_keywords option_args single_args multi_args)
    set(${option_args}
        STATIC
        SHARED
    )
    set(${single_args}
        TYPE
        CLASS_NAME
        OUTPUT_NAME
    )
    set(${multi_args})
endmacro()

# This function is currently in Technical Preview.
# It's signature and behavior might change.
function(qt6_add_plugin target)
    _qt_internal_get_add_plugin_keywords(opt_args single_args multi_args)

    # TODO: Transitional use only, replaced by CLASS_NAME. Remove this once
    #       all other repos have been updated to use CLASS_NAME.
    list(APPEND single_args CLASSNAME)

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")

    # Handle the inconsistent CLASSNAME/CLASS_NAME keyword naming between commands
    if(arg_CLASSNAME)
        if(arg_CLASS_NAME AND NOT arg_CLASSNAME STREQUAL arg_CLASS_NAME)
            message(FATAL_ERROR
                "Both CLASSNAME and CLASS_NAME were given and were different. "
                "Only one of the two should be used."
            )
        endif()
        set(arg_CLASS_NAME "${arg_CLASSNAME}")
        unset(arg_CLASSNAME)
    endif()

    if(arg_STATIC AND arg_SHARED)
        message(FATAL_ERROR
            "Both STATIC and SHARED options were given. Only one of the two should be used."
        )
    endif()

    # If no explicit STATIC/SHARED option is set, default to the flavor of the Qt build.
    if(QT6_IS_SHARED_LIBS_BUILD)
        set(create_static_plugin FALSE)
    else()
        set(create_static_plugin TRUE)
    endif()

    # Explicit option takes priority over the computed default.
    if(arg_STATIC)
        set(create_static_plugin TRUE)
    elseif(arg_SHARED)
        set(create_static_plugin FALSE)
    endif()

    if (create_static_plugin)
        add_library(${target} STATIC)
        target_compile_definitions(${target} PRIVATE QT_STATICPLUGIN)
    else()
        add_library(${target} MODULE)
        if(APPLE)
            # CMake defaults to using .so extensions for loadable modules, aka plugins,
            # but Qt plugins are actually suffixed with .dylib.
            set_property(TARGET "${target}" PROPERTY SUFFIX ".dylib")
        endif()
        _qt_internal_apply_win_prefix_and_suffix(${target})
    endif()

    set(output_name ${target})
    if (arg_OUTPUT_NAME)
        set(output_name ${arg_OUTPUT_NAME})
    endif()
    set_property(TARGET "${target}" PROPERTY OUTPUT_NAME "${output_name}")

    if (ANDROID)
        qt6_android_apply_arch_suffix("${target}")
        set_target_properties(${target}
            PROPERTIES
            LIBRARY_OUTPUT_NAME "plugins_${arg_TYPE}_${output_name}"
        )
    endif()

    # Derive the class name from the target name if it's not explicitly specified.
    # Don't set it for qml plugins though.
    set(plugin_class_name "")
    if (NOT "${arg_TYPE}" STREQUAL "qml_plugin")
        if (NOT arg_CLASS_NAME)
            set(plugin_class_name "${target}")
        else()
            set(plugin_class_name "${arg_CLASS_NAME}")
        endif()
    endif()
    set_target_properties(${target} PROPERTIES QT_PLUGIN_CLASS_NAME "${plugin_class_name}")

    target_compile_definitions(${target} PRIVATE
        QT_PLUGIN
        QT_DEPRECATED_WARNINGS
    )
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_plugin)
        qt6_add_plugin(${ARGV})
    endfunction()
endif()

# By default Qt6 forces usage of utf8 sources for consumers of Qt.
# Users can opt out of utf8 sources by calling this function with the target name of their
# application or library.
function(qt6_allow_non_utf8_sources target)
    set_target_properties("${target}" PROPERTIES QT_NO_UTF8_SOURCE TRUE)
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_allow_non_utf8_sources)
        qt6_allow_non_utf8_sources(${ARGV})
    endfunction()
endif()

function(_qt_internal_apply_strict_cpp target)
    # Disable C, Obj-C and C++ GNU extensions aka no "-std=gnu++11".
    # Similar to mkspecs/features/default_post.prf's CONFIG += strict_cpp.
    # Allow opt-out via variable.
    if(NOT QT_ENABLE_CXX_EXTENSIONS)
        get_target_property(target_type "${target}" TYPE)
        if(NOT target_type STREQUAL "INTERFACE_LIBRARY")
            set_target_properties("${target}" PROPERTIES
                CXX_EXTENSIONS OFF
                C_EXTENSIONS OFF
                OBJC_EXTENSIONS OFF
                OBJCXX_EXTENSIONS OFF)
        endif()
    endif()
endfunction()
