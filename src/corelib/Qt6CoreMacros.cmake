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

# macro used to create the names of output files preserving relative dirs
macro(qt6_make_output_file infile prefix ext outfile )
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


macro(qt6_get_moc_flags _moc_flags)
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
function(qt6_create_moc_command infile outfile moc_flags moc_options moc_target moc_depends)
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
    qt6_get_moc_flags(moc_flags)
    get_filename_component(abs_infile ${infile} ABSOLUTE)
    set(_outfile "${outfile}")
    if(NOT IS_ABSOLUTE "${outfile}")
        set(_outfile "${CMAKE_CURRENT_BINARY_DIR}/${outfile}")
    endif()
    if ("x${ARGV2}" STREQUAL "xTARGET")
        set(moc_target ${ARGV3})
    endif()
    qt6_create_moc_command(${abs_infile} ${_outfile} "${moc_flags}" "" "${moc_target}" "")
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
    qt6_get_moc_flags(moc_flags)

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
        qt6_make_output_file(${it} moc_ cpp outfile)
        qt6_create_moc_command(${it} ${outfile} "${moc_flags}" "${moc_options}" "${moc_target}" "${moc_depends}")
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
        qt6_make_output_file("${infile}" "" "qrc.depends" out_depends)
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
        qt6_process_resource(${ARGV})
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
        set_target_properties(rcc_object_${outfilename} PROPERTIES AUTOMOC OFF)
        set_target_properties(rcc_object_${outfilename} PROPERTIES AUTOUIC OFF)
        add_dependencies(rcc_object_${outfilename} big_resources_${outfilename})
        # The modification of TARGET_OBJECTS needs the following change in cmake
        # https://gitlab.kitware.com/cmake/cmake/commit/93c89bc75ceee599ba7c08b8fe1ac5104942054f
        add_custom_command(OUTPUT ${outfile}
                           COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::rcc
                           ARGS ${rcc_options} --name ${outfilename} --pass 2 --temp $<TARGET_OBJECTS:rcc_object_${outfilename}> --output ${outfile} ${infile}
                           DEPENDS rcc_object_${outfilename} ${QT_CMAKE_EXPORT_NAMESPACE}::rcc
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

set(_Qt6_COMPONENT_PATH "${CMAKE_CURRENT_LIST_DIR}/..")

macro(qt6_use_modules _target _link_type)
    if(CMAKE_WARN_DEPRECATED)
        set(messageType WARNING)
    endif()
    if(CMAKE_ERROR_DEPRECATED)
        set(messageType FATAL_ERROR)
    endif()
    if(messageType)
        message(${messageType} "The qt6_use_modules macro is obsolete. Use target_link_libraries with IMPORTED targets instead.")
    endif()

    if (NOT TARGET ${_target})
        message(FATAL_ERROR "The first argument to qt6_use_modules must be an existing target.")
    endif()
    if ("${_link_type}" STREQUAL "LINK_PUBLIC" OR "${_link_type}" STREQUAL "LINK_PRIVATE" )
        set(_qt6_modules ${ARGN})
        set(_qt6_link_type ${_link_type})
    else()
        set(_qt6_modules ${_link_type} ${ARGN})
    endif()

    if ("${_qt6_modules}" STREQUAL "")
        message(FATAL_ERROR "qt6_use_modules requires at least one Qt module to use.")
    endif()

    foreach(_module ${_qt6_modules})
        if (NOT Qt6${_module}_FOUND)
            find_package(Qt6${_module} PATHS "${_Qt6_COMPONENT_PATH}" NO_DEFAULT_PATH)
            if (NOT Qt6${_module}_FOUND)
                message(FATAL_ERROR "Cannot use \"${_module}\" module which has not yet been found.")
            endif()
        endif()
        target_link_libraries(${_target} ${_qt6_link_type} ${Qt6${_module}_LIBRARIES})
        set_property(TARGET ${_target} APPEND PROPERTY INCLUDE_DIRECTORIES ${Qt6${_module}_INCLUDE_DIRS})
        set_property(TARGET ${_target} APPEND PROPERTY COMPILE_DEFINITIONS ${Qt6${_module}_COMPILE_DEFINITIONS})
        set_property(TARGET ${_target} APPEND PROPERTY COMPILE_DEFINITIONS_RELEASE QT_NO_DEBUG)
        set_property(TARGET ${_target} APPEND PROPERTY COMPILE_DEFINITIONS_RELWITHDEBINFO QT_NO_DEBUG)
        set_property(TARGET ${_target} APPEND PROPERTY COMPILE_DEFINITIONS_MINSIZEREL QT_NO_DEBUG)
        if (Qt6_POSITION_INDEPENDENT_CODE
                    AND (NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
                    OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0))
            set_property(TARGET ${_target} PROPERTY POSITION_INDEPENDENT_CODE ${Qt6_POSITION_INDEPENDENT_CODE})
        endif()
    endforeach()
endmacro()

function(add_qt_gui_executable target)
    if(ANDROID)
        add_library("${target}" MODULE ${ARGN})
        # On our qmake builds we do don't compile the executables with
        # visibility=hidden. Not having this flag set will cause the
        # executable to have main() hidden and can then no longer be loaded
        # through dlopen()
        set_property(TARGET "${target}" PROPERTY C_VISIBILITY_PRESET default)
        set_property(TARGET "${target}" PROPERTY CXX_VISIBILITY_PRESET default)
        qt_android_apply_arch_suffix("${target}")
    else()
        add_executable("${target}" WIN32 MACOSX_BUNDLE ${ARGN})
    endif()
    target_link_libraries("${target}" PRIVATE Qt::Core)
    if(TARGET Qt::Gui)
        target_link_libraries("${target}" PRIVATE Qt::Gui)
    endif()

    if(ANDROID)
        qt_android_generate_deployment_settings("${target}")
        qt_android_add_apk_target("${target}")
    endif()
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
        set_target_properties(${target} PROPERTIES QT_DEFAULT_PLUGINS 0)
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


# Generate Qt metatypes.json for a target. By default we check whether AUTOMOC
# has been enabled and we extract the information from that target. Should you
# not wish to use automoc you need to pass in all the generated json files via the
# MANUAL_MOC_JSON_FILES parameter. The latter can be obtained by running moc with
# the --output-json parameter.
# Params:
#   INSTALL_DIR: Location where to install the metatypes file (Optional)
#   COPY_OVER_INSTALL: When present will install the file via a post build step
#   copy rather than using install
function(qt6_generate_meta_types_json_file target)

    get_target_property(existing_meta_types_file ${target} INTERFACE_QT_META_TYPES_BUILD_FILE)
    if (existing_meta_types_file)
        return()
    endif()

    cmake_parse_arguments(arg "COPY_OVER_INSTALL" "INSTALL_DIR" "MANUAL_MOC_JSON_FILES" ${ARGN})

    if (NOT QT_BUILDING_QT)
        if (NOT arg_INSTALL_DIR)
            message(FATAL_ERROR "Please specify an install directory using INSTALL_DIR")
        endif()
    else()
        # Automatically fill install args when building qt
        set(metatypes_install_dir ${INSTALL_LIBDIR}/metatypes)
        set(args)
        if (NOT QT_WILL_INSTALL)
            set(arg_COPY_OVER_INSTALL TRUE)
        endif()
        if (NOT arg_INSTALL_DIR)
            set(arg_INSTALL_DIR "${metatypes_install_dir}")
        endif()
    endif()

    get_target_property(target_type ${target} TYPE)
    if (target_type STREQUAL "INTERFACE_LIBRARY")
        message(FATAL_ERROR "Meta types generation does not work on interface libraries")
        return()
    endif()

    if (CMAKE_VERSION VERSION_LESS "3.16.0")
        message(FATAL_ERROR "Meta types generation requires CMake >= 3.16")
        return()
    endif()

    get_target_property(target_binary_dir ${target} BINARY_DIR)
    set(type_list_file "${target_binary_dir}/meta_types/${target}_json_file_list.txt")
    set(type_list_file_manual "${target_binary_dir}/meta_types/${target}_json_file_list_manual.txt")

    get_target_property(uses_automoc ${target} AUTOMOC)
    set(automoc_args)
    set(automoc_dependencies)
    #Handle automoc generated data
    if (uses_automoc)
        # Tell automoc to output json files)
        set_property(TARGET "${target}" APPEND PROPERTY
            AUTOMOC_MOC_OPTIONS "--output-json"
        )

        if(CMAKE_BUILD_TYPE)
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
    if (arg_COPY_OVER_INSTALL AND NOT EXISTS ${arg_INSTALL_DIR}/${metatypes_file_name})
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
        COMMENT "Runing automoc with --collect-json"
    )

    # We still need to add this file as a source of Core, otherwise the file
    # rule above is not triggered. INTERFACE_SOURCES do not properly register
    # as dependencies to build the current target.
    target_sources(${target} PRIVATE ${metatypes_file_gen})
    set(metatypes_file_genex_build)
    set(metatypes_file_genex_install)
    if (arg_COPY_OVER_INSTALL)
        set(metatypes_file_genex_build
            "$<BUILD_INTERFACE:$<$<BOOL:$<TARGET_PROPERTY:QT_CONSUMES_METATYPES>>:${arg_INSTALL_DIR}/${metatypes_file_name}>>"
        )
    else()
        set(metatypes_file_genex_build
            "$<BUILD_INTERFACE:$<$<BOOL:$<TARGET_PROPERTY:QT_CONSUMES_METATYPES>>:${metatypes_file}>>"
        )
        set(metatypes_file_genex_install
            "$<INSTALL_INTERFACE:$<$<BOOL:$<TARGET_PROPERTY:QT_CONSUMES_METATYPES>>:$<INSTALL_PREFIX>/${arg_INSTALL_DIR}/${metatypes_file_name}>>"
        )
    endif()
    set_source_files_properties(${metatypes_file} PROPERTIES HEADER_FILE_ONLY TRUE)

    set_target_properties(${target} PROPERTIES
        INTERFACE_QT_MODULE_HAS_META_TYPES YES
        INTERFACE_QT_MODULE_META_TYPES_FROM_BUILD YES
        INTERFACE_QT_META_TYPES_BUILD_FILE ${metatypes_file}
        QT_MODULE_META_TYPES_FILE_GENEX_BUILD "${metatypes_file_genex_build}"
        QT_MODULE_META_TYPES_FILE_GENEX_INSTALL "${metatypes_file_genex_install}"
    )
    target_sources(${target} INTERFACE ${metatypes_file_genex_build} ${metatypes_file_genex_install})

    if (arg_COPY_OVER_INSTALL)
        get_target_property(target_type ${target} TYPE)
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
        install(FILES "${metatypes_file}"
            DESTINATION "${arg_INSTALL_DIR}"
        )
    endif()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_generate_meta_types_json_file)
        qt6_generate_meta_types_json_file(${ARGV})
    endfunction()
endif()
