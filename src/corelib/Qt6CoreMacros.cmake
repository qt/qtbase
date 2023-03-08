# Copyright 2005-2011 Kitware, Inc.
# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

######################################
#
#       Macros for building Qt files
#
######################################

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
function(_qt_internal_create_moc_command infile outfile moc_flags moc_options
         moc_target moc_depends out_json_file)
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
    if(out_json_file)
        list(APPEND _moc_parameters --output-json)
        set(extra_output_files "${outfile}.json")
        set(${out_json_file} "${extra_output_files}" PARENT_SCOPE)
    endif()
    string (REPLACE ";" "\n" _moc_parameters "${_moc_parameters}")

    if(moc_target)
        set(_moc_parameters_file ${_moc_parameters_file}$<$<BOOL:$<CONFIG>>:_$<CONFIG>>)
        set(targetincludes "$<TARGET_PROPERTY:${moc_target},INCLUDE_DIRECTORIES>")
        set(targetdefines "$<TARGET_PROPERTY:${moc_target},COMPILE_DEFINITIONS>")

        set(targetincludes "$<$<BOOL:${targetincludes}>:-I$<JOIN:$<REMOVE_DUPLICATES:${targetincludes}>,\n-I>\n>")
        set(targetdefines "$<$<BOOL:${targetdefines}>:-D$<JOIN:$<REMOVE_DUPLICATES:${targetdefines}>,\n-D>\n>")

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
    add_custom_command(OUTPUT ${outfile} ${extra_output_files}
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
    _qt_internal_create_moc_command(${abs_infile} ${_outfile} "${moc_flags}" "" "${moc_target}"
        "" # moc_depends
        "" # out_json_file
    )
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
    set(oneValueArgs
        TARGET
        __QT_INTERNAL_OUTPUT_MOC_JSON_FILES
    )
    set(multiValueArgs OPTIONS DEPENDS)

    cmake_parse_arguments(_WRAP_CPP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(moc_files ${_WRAP_CPP_UNPARSED_ARGUMENTS})
    set(moc_options ${_WRAP_CPP_OPTIONS})
    set(moc_target ${_WRAP_CPP_TARGET})
    set(moc_depends ${_WRAP_CPP_DEPENDS})

    set(metatypes_json_list "")

    foreach(it ${moc_files})
        get_filename_component(it ${it} ABSOLUTE)
        _qt_internal_make_output_file(${it} moc_ cpp outfile)

        set(out_json_file_var "")
        if(_WRAP_CPP___QT_INTERNAL_OUTPUT_MOC_JSON_FILES)
            set(out_json_file_var "out_json_file")
        endif()

        _qt_internal_create_moc_command(
            ${it} ${outfile} "${moc_flags}" "${moc_options}" "${moc_target}" "${moc_depends}"
            "${out_json_file_var}")
        list(APPEND ${outfiles} ${outfile})
        if(_WRAP_CPP___QT_INTERNAL_OUTPUT_MOC_JSON_FILES)
            list(APPEND metatypes_json_list "${${out_json_file_var}}")
        endif()
    endforeach()
    set(${outfiles} ${${outfiles}} PARENT_SCOPE)

    if(metatypes_json_list)
        set(${_WRAP_CPP___QT_INTERNAL_OUTPUT_MOC_JSON_FILES}
            "${metatypes_json_list}" PARENT_SCOPE)
    endif()
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

    if(NOT QT_FEATURE_zstd)
        list(APPEND rcc_options "--no-zstd")
    endif()

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

        if(NOT QT_FEATURE_zstd)
            list(APPEND rcc_options "--no-zstd")
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
            set_source_files_properties(${outfile} PROPERTIES SKIP_AUTOMOC ON
                                                              SKIP_AUTOUIC ON
                                                              SKIP_UNITY_BUILD_INCLUSION ON
                                                              SKIP_PRECOMPILE_HEADERS ON
                                                              )
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
        if(TARGET ${outfiles})
            cmake_parse_arguments(PARSE_ARGV 1 arg "" "OUTPUT_TARGETS" "")
            if (arg_OUTPUT_TARGETS)
                set(${arg_OUTPUT_TARGETS} ${${arg_OUTPUT_TARGETS}} PARENT_SCOPE)
            endif()
        else()
            set("${outfiles}" "${${outfiles}}" PARENT_SCOPE)
        endif()
    endfunction()
endif()


# qt6_add_big_resources(outfiles inputfile ... )

function(qt6_add_big_resources outfiles )
    if(CMAKE_GENERATOR STREQUAL "Xcode" AND IOS)
        message(WARNING
            "Due to CMake limitations, qt6_add_big_resources can't be used when building for iOS. "
            "See https://bugreports.qt.io/browse/QTBUG-103497 for details. "
            "Falling back to using qt6_add_resources. "
            "Consider using qt6_add_resources directly to silence this warning."
        )
        qt6_add_resources(${ARGV})
        set(${outfiles} ${${outfiles}} PARENT_SCOPE)
        return()
    endif()

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

    if(NOT QT_FEATURE_zstd)
        list(APPEND rcc_options "--no-zstd")
    endif()

    foreach(it ${rcc_files})
        get_filename_component(outfilename ${it} NAME_WE)

        # Provide unique targets and output file names
        # in case we add multiple .qrc files with the same base name.
        string(MAKE_C_IDENTIFIER "_qt_big_resource_count_${outfilename}" prop)
        get_property(count GLOBAL PROPERTY ${prop})
        if(count)
            string(APPEND outfilename "_${count}")
        else()
            set(count 0)
        endif()
        math(EXPR count "${count} + 1")
        set_property(GLOBAL PROPERTY ${prop} ${count})

        get_filename_component(infile ${it} ABSOLUTE)
        set(tmpoutfile ${CMAKE_CURRENT_BINARY_DIR}/qrc_${outfilename}tmp.cpp)
        set(outfile ${CMAKE_CURRENT_BINARY_DIR}/qrc_${outfilename}.o)

        _qt6_parse_qrc_file(${infile} _out_depends _rc_depends)
        set_source_files_properties(${infile} PROPERTIES SKIP_AUTOGEN ON)
        add_custom_command(OUTPUT ${tmpoutfile}
                           COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::rcc ${rcc_options} --name ${outfilename} --pass 1 --output ${tmpoutfile} ${infile}
                           DEPENDS ${infile} ${_rc_depends} "${out_depends}" ${QT_CMAKE_EXPORT_NAMESPACE}::rcc
                           COMMENT "Running rcc pass 1 for resource ${outfilename}"
                           VERBATIM)
        add_custom_target(big_resources_${outfilename} ALL DEPENDS ${tmpoutfile})
        _qt_internal_add_rcc_pass2(
            RESOURCE_NAME ${outfilename}
            RCC_OPTIONS ${rcc_options}
            OBJECT_LIB rcc_object_${outfilename}
            QRC_FILE ${infile}
            PASS1_OUTPUT_FILE ${tmpoutfile}
            OUT_OBJECT_FILE ${outfile}
        )
        add_dependencies(rcc_object_${outfilename} big_resources_${outfilename})
        list(APPEND ${outfiles} ${outfile})
    endforeach()
    set(${outfiles} ${${outfiles}} PARENT_SCOPE)
endfunction()

function(_qt_internal_add_rcc_pass2)
    set(options)
    set(oneValueArgs
        RESOURCE_NAME
        OBJECT_LIB
        QRC_FILE
        PASS1_OUTPUT_FILE
        OUT_OBJECT_FILE
    )
    set(multiValueArgs
        RCC_OPTIONS
    )
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_library(${arg_OBJECT_LIB} OBJECT ${arg_PASS1_OUTPUT_FILE})
    _qt_internal_set_up_static_runtime_library(${arg_OBJECT_LIB})
    target_compile_definitions(${arg_OBJECT_LIB} PUBLIC
        "$<TARGET_PROPERTY:Qt6::Core,INTERFACE_COMPILE_DEFINITIONS>")
    set_target_properties(${arg_OBJECT_LIB} PROPERTIES
        AUTOMOC OFF
        AUTOUIC OFF)
    # The modification of TARGET_OBJECTS needs the following change in cmake
    # https://gitlab.kitware.com/cmake/cmake/commit/93c89bc75ceee599ba7c08b8fe1ac5104942054f
    add_custom_command(
        OUTPUT ${arg_OUT_OBJECT_FILE}
        COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::rcc
                ${arg_RCC_OPTIONS} --name ${arg_RESOURCE_NAME} --pass 2
                --temp $<TARGET_OBJECTS:${arg_OBJECT_LIB}>
                --output ${arg_OUT_OBJECT_FILE} ${arg_QRC_FILE}
        DEPENDS ${arg_OBJECT_LIB} $<TARGET_OBJECTS:${arg_OBJECT_LIB}>
                ${QT_CMAKE_EXPORT_NAMESPACE}::rcc
        COMMENT "Running rcc pass 2 for resource ${arg_RESOURCE_NAME}"
        VERBATIM)
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

# Wrapper function that adds an executable with some Qt specific behavior.
# Some scenarios require steps to be deferred to the end of the current
# directory scope so that the caller has an opportunity to modify certain
# target properties.
function(qt6_add_executable target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "MANUAL_FINALIZATION" "" "")

    _qt_internal_create_executable("${target}" ${arg_UNPARSED_ARGUMENTS})
    target_link_libraries("${target}" PRIVATE Qt6::Core)
    set_property(TARGET ${target} PROPERTY _qt_expects_finalization TRUE)

    if(arg_MANUAL_FINALIZATION)
        # Caller says they will call qt6_finalize_target() themselves later
        return()
    endif()

    # Defer the finalization if we can. When the caller's project requires
    # CMake 3.19 or later, this makes the calls to this function concise while
    # still allowing target property modification before finalization.
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
        # Need to wrap in an EVAL CODE or else ${target} won't be evaluated
        # due to special behavior of cmake_language() argument handling
        cmake_language(EVAL CODE "cmake_language(DEFER CALL qt6_finalize_target ${target})")
    else()
        set_target_properties("${target}" PROPERTIES _qt_is_immediately_finalized TRUE)
        qt6_finalize_target("${target}")
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
        set_property(TARGET "${target}"
                     PROPERTY _qt_android_apply_arch_suffix_called_from_qt_impl TRUE)
        qt6_android_apply_arch_suffix("${target}")
        set_property(TARGET "${target}" PROPERTY _qt_is_android_executable TRUE)
    else()
        add_executable("${target}" ${ARGN})
    endif()

    _qt_internal_set_up_static_runtime_library("${target}")
endfunction()

function(_qt_internal_finalize_executable target)
    # We can't evaluate generator expressions at configure time, so we can't
    # ask for any transitive properties or even the full library dependency
    # chain.
    # We can still look at the immediate dependencies
    # (and recursively their dependencies) and query
    # any that are not expressed as generator expressions. For any we can
    # identify as a CMake target known to the current scope, we can check if
    # that target has a finalizer to be called. This is expected to cover the
    # vast majority of use cases, since projects will typically link directly
    # to Qt::* targets. For the cases where this isn't so, the project will be
    # responsible for calling any relevant functions themselves instead of
    # relying on these automatic finalization calls.
    set(finalizers)

    __qt_internal_collect_all_target_dependencies("${target}" dep_targets)

    if(dep_targets)
        foreach(dep IN LISTS dep_targets)
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
            # cmake_language() not available, fall back to the slower method of
            # writing a file and including it
            set(contents "")
            foreach(finalizer_func IN LISTS finalizers)
                string(APPEND contents "${finalizer_func}(${target})\n")
            endforeach()
            set(finalizer_file "${CMAKE_CURRENT_BINARY_DIR}/.qt/finalize_${target}.cmake")
            file(WRITE ${finalizer_file} "${contents}")
            include(${finalizer_file})
        else()
            foreach(finalizer_func IN LISTS finalizers)
                cmake_language(CALL ${finalizer_func} ${target})
            endforeach()
        endif()
    endif()

    if(EMSCRIPTEN)
        _qt_internal_wasm_add_target_helpers("${target}")
        _qt_internal_add_wasm_extra_exported_methods("${target}")
    endif()
    if(IOS)
        _qt_internal_finalize_ios_app("${target}")
    elseif(APPLE)
        _qt_internal_finalize_macos_app("${target}")
    endif()

    # For finalizer mode of plugin importing to work safely, we need to know the list of Qt
    # dependencies the target has, but those might be added later than the qt_add_executable call.
    # Most of our examples are like that. Only enable finalizer mode when we are sure that the user
    # manually called qt_finalize_target at the end of their CMake project, or it was automatically
    # done via a deferred call. This is also applicable to the resource object finalizer.
    get_target_property(is_immediately_finalized "${target}" _qt_is_immediately_finalized)
    if(NOT is_immediately_finalized)
        __qt_internal_apply_plugin_imports_finalizer_mode("${target}")
        __qt_internal_process_dependency_object_libraries("${target}")
    endif()
endfunction()

# If a task needs to run before any targets are finalized in the current directory
# scope, call this function and pass the ID of that task as the argument.
function(_qt_internal_delay_finalization_until_after defer_id)
    set_property(DIRECTORY APPEND PROPERTY qt_internal_finalizers_wait_for_ids "${defer_id}")
endfunction()

function(qt6_finalize_target target)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
        cmake_language(DEFER GET_CALL_IDS ids_queued)
        get_directory_property(wait_for_ids qt_internal_finalizers_wait_for_ids)
        while(wait_for_ids)
            list(GET wait_for_ids 0 id_to_wait_for)
            if(id_to_wait_for IN_LIST ids_queued)
                # Something else needs to run before we finalize targets.
                # Try again later by re-deferring ourselves, which effectively
                # puts us at the end of the current list of deferred actions.
                cmake_language(EVAL CODE "cmake_language(DEFER CALL ${CMAKE_CURRENT_FUNCTION} ${ARGV})")
                set_directory_properties(PROPERTIES
                    qt_internal_finalizers_wait_for_ids "${wait_for_ids}"
                )
                return()
            endif()
            list(POP_FRONT wait_for_ids)
        endwhile()
        # No other deferred tasks to wait for
        set_directory_properties(PROPERTIES qt_internal_finalizers_wait_for_ids "")
    endif()

    if(NOT TARGET "${target}")
        message(FATAL_ERROR "No target '${target}' found in current scope.")
    endif()

    get_target_property(is_finalized "${target}" _qt_is_finalized)
    if(is_finalized)
        message(AUTHOR_WARNING
            "Tried to call qt6_finalize_target twice on target '${target}'. "
            "Did you forget to specify MANUAL_FINALIZATION to qt6_add_executable, "
            "qt6_add_library or qt6_add_plugin?")
        return()
    endif()

    _qt_internal_expose_deferred_files_to_ide(${target})
    get_target_property(target_type ${target} TYPE)
    get_target_property(is_android_executable "${target}" _qt_is_android_executable)

    if(target_type STREQUAL "EXECUTABLE" OR is_android_executable)
        _qt_internal_finalize_executable(${ARGV})
    endif()

    set_target_properties(${target} PROPERTIES _qt_is_finalized TRUE)
endfunction()

function(_qt_internal_darwin_permission_finalizer target)
    get_target_property(plist_file "${target}" MACOSX_BUNDLE_INFO_PLIST)
    if(NOT plist_file)
        return()
    endif()
    foreach(plugin_target IN LISTS QT_ALL_PLUGINS_FOUND_BY_FIND_PACKAGE_permissions)
        set(versioned_plugin_target "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_target}")
        get_target_property(usage_descriptions
            ${versioned_plugin_target}
            _qt_info_plist_usage_descriptions)
        foreach(usage_description_key IN LISTS usage_descriptions)
            execute_process(COMMAND "/usr/libexec/PlistBuddy"
                -c "print ${usage_description_key}" "${plist_file}"
                OUTPUT_VARIABLE usage_description
                ERROR_VARIABLE plist_error)
            if(usage_description AND NOT plist_error)
                set_target_properties("${target}"
                    PROPERTIES "_qt_has_${plugin_target}_usage_description" TRUE)
                qt6_import_plugins(${target} INCLUDE ${versioned_plugin_target})
            endif()
        endforeach()
    endforeach()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_executable)
        qt6_add_executable(${ARGV})
    endfunction()
    function(qt_finalize_target)
        qt6_finalize_target(${ARGV})
    endfunction()

    # Kept for compatibility with Qt Creator 4.15 wizards
    function(qt_finalize_executable)
        qt6_finalize_target(${ARGV})
    endfunction()
endif()

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
            set_property(TARGET "${target}" APPEND PROPERTY "QT_PLUGINS_${_current_type}" "${_arg}")
            set_property(TARGET "${target}" APPEND PROPERTY "_qt_plugins_by_type" "${_arg}")
            _qt_get_plugin_name_with_version("${_arg}" qt_plugin_with_version)

            # TODO: Do we really need this check? We didn't have it in Qt5, and plugin targets
            # wrapped in genexes end up causing warnings, but we explicitly use GENEX_EVAL to
            # support them.
            if(NOT TARGET "${_arg}" AND NOT TARGET "${qt_plugin_with_version}")
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

# This function is currently in Technical Preview. It's signature may change or be removed entirely.
function(qt6_set_finalizer_mode target)
    cmake_parse_arguments(arg "ENABLE;DISABLE" "" "MODES" ${ARGN})
    if(NOT arg_ENABLE AND NOT arg_DISABLE)
        message(FATAL_ERROR "No option was specified whether to enable or disable the modes.")
    elseif(arg_ENABLE AND arg_DISABLE)
        message(FATAL_ERROR "Both ENABLE and DISABLE options were specified.")
    endif()
    if(NOT arg_MODES)
        message(FATAL_ERROR "No modes were specified in qt6_set_finalizer_mode() call.")
    endif()

    if(arg_ENABLE)
        set(value "TRUE")
    elseif(arg_DISABLE)
        set(value "FALSE")
    endif()

    foreach(mode ${arg_MODES})
        __qt_internal_enable_finalizer_mode("${target}" "${mode}" "${value}")
    endforeach()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_set_finalizer_mode)
        qt6_set_finalizer_mode(${ARGV})
    endfunction()
endif()

function(_qt_internal_assign_to_internal_targets_folder target)
    get_property(folder_name GLOBAL PROPERTY QT_TARGETS_FOLDER)
    if(NOT "${folder_name}" STREQUAL "")
        set_property(TARGET ${target} PROPERTY FOLDER "${folder_name}")
    endif()
endfunction()

function(_qt_internal_get_target_autogen_build_dir target out_var)
    get_property(target_autogen_build_dir TARGET ${target} PROPERTY AUTOGEN_BUILD_DIR)
    if(target_autogen_build_dir)
        set(${out_var} "${target_autogen_build_dir}" PARENT_SCOPE)
    else()
        get_property(target_binary_dir TARGET ${target} PROPERTY BINARY_DIR)
        set(${out_var} "${target_binary_dir}/${target}_autogen" PARENT_SCOPE)
    endif()
endfunction()

function(qt6_extract_metatypes target)

    get_target_property(existing_meta_types_file ${target} INTERFACE_QT_META_TYPES_BUILD_FILE)
    if (existing_meta_types_file)
        return()
    endif()

    set(args_option
        # TODO: Move this into a separate internal function, so it doesn't pollute the public one.
        # When given, metatypes files will be installed into the default Qt
        # metatypes folder. Only to be used by the Qt build.
        __QT_INTERNAL_INSTALL
    )
    set(args_single
        # TODO: Move this into a separate internal function, so it doesn't pollute the public one.
        # Location where to install the metatypes file. Only used if
        # __QT_INTERNAL_INSTALL is given. It defaults to the
        # ${CMAKE_INSTALL_PREFIX}/${INSTALL_ARCHDATADIR}/metatypes directory.
        # Executable metatypes files are never installed.
        __QT_INTERNAL_INSTALL_DIR

        OUTPUT_FILES
    )
    set(args_multi
        MANUAL_MOC_JSON_FILES
    )

    cmake_parse_arguments(arg
        "${args_option}"
        "${args_single}"
        "${args_multi}" ${ARGN})

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

    set(target_autogen_build_dir "")
    _qt_internal_get_target_autogen_build_dir(${target} target_autogen_build_dir)

    get_target_property(uses_automoc ${target} AUTOMOC)
    set(automoc_args)
    set(automoc_dependencies)
    # Handle automoc generated data
    if (uses_automoc)
        # Tell automoc to output json files)
        set_property(TARGET "${target}" APPEND PROPERTY
            AUTOMOC_MOC_OPTIONS "--output-json"
        )

        get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
        if(NOT is_multi_config)
            set(cmake_autogen_cache_file
                "${target_binary_dir}/CMakeFiles/${target}_autogen.dir/ParseCache.txt")
            set(multi_config_args
                --cmake-autogen-include-dir-path "${target_autogen_build_dir}/include"
            )
        else()
            set(cmake_autogen_cache_file
                "${target_binary_dir}/CMakeFiles/${target}_autogen.dir/ParseCache_$<CONFIG>.txt")
            set(multi_config_args
                --cmake-autogen-include-dir-path "${target_autogen_build_dir}/include_$<CONFIG>"
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

        set(cmake_automoc_parser_timestamp "${type_list_file}.timestamp")

        if (NOT use_dep_files)
            # When a project is configured with a Visual Studio generator, CMake's
            # cmQtAutoGenInitializer::InitAutogenTarget() can take one of two code paths on how to
            # handle AUTOMOC rules.
            # It either creates a ${target}_autogen custom target or uses PRE_BUILD build events.
            #
            # The latter in considered an optimization and is used by CMake when possible.
            # Unfortunately that breaks our add_dependency call because we expect on _autogen target
            # to always exist.
            #
            # Ensure the PRE_BUILD path is not taken by generating a dummy header file and adding it
            # as a source file to the target. This causes the file to be added to
            # cmQtAutoGenInitializer::AutogenTarget.DependFiles, which disables the PRE_BUILD path.
            if(CMAKE_GENERATOR MATCHES "Visual Studio")
                # The file name should be target specific, but still short, so we don't hit path
                # length issues.
                string(MAKE_C_IDENTIFIER "ddf_${target}" dummy_dependency_file)
                set(dummy_out_file "${CMAKE_CURRENT_BINARY_DIR}/${dummy_dependency_file}.h")

                # The content shouldn't be empty so we don't trigger AUTOMOC warnings about it.
                file(GENERATE OUTPUT "${dummy_out_file}" CONTENT "//")
                set_source_files_properties("${dummy_out_file}" PROPERTIES
                    GENERATED TRUE
                    SKIP_AUTOGEN OFF)
                target_sources("${target}" PRIVATE "${dummy_out_file}")
            endif()

            add_custom_target(${target}_automoc_json_extraction
                DEPENDS ${QT_CMAKE_EXPORT_NAMESPACE}::cmake_automoc_parser
                BYPRODUCTS
                    ${type_list_file}
                    "${cmake_automoc_parser_timestamp}"
                COMMAND
                    ${QT_CMAKE_EXPORT_NAMESPACE}::cmake_automoc_parser
                    --cmake-autogen-cache-file "${cmake_autogen_cache_file}"
                    --cmake-autogen-info-file "${cmake_autogen_info_file}"
                    --output-file-path "${type_list_file}"
                    --timestamp-file-path "${cmake_automoc_parser_timestamp}"
                    ${multi_config_args}
                COMMENT "Running AUTOMOC file extraction for target ${target}"
                COMMAND_EXPAND_LISTS
            )
            add_dependencies(${target}_automoc_json_extraction ${target}_autogen)
            _qt_internal_assign_to_internal_targets_folder(${target}_automoc_json_extraction)
        else()
            set(cmake_autogen_timestamp_file
                "${target_autogen_build_dir}/timestamp"
            )

            add_custom_command(OUTPUT ${type_list_file}
                DEPENDS ${QT_CMAKE_EXPORT_NAMESPACE}::cmake_automoc_parser
                    ${cmake_autogen_timestamp_file}
                BYPRODUCTS "${cmake_automoc_parser_timestamp}"
                COMMAND
                    ${QT_CMAKE_EXPORT_NAMESPACE}::cmake_automoc_parser
                    --cmake-autogen-cache-file "${cmake_autogen_cache_file}"
                    --cmake-autogen-info-file "${cmake_autogen_info_file}"
                    --output-file-path "${type_list_file}"
                    --timestamp-file-path "${cmake_automoc_parser_timestamp}"
                    ${multi_config_args}
                COMMENT "Running AUTOMOC file extraction for target ${target}"
                COMMAND_EXPAND_LISTS
                VERBATIM
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

    if (CMAKE_BUILD_TYPE AND NOT is_multi_config)
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

    add_custom_command(
        OUTPUT
            ${metatypes_file_gen}
            ${metatypes_file}
        DEPENDS ${QT_CMAKE_EXPORT_NAMESPACE}::moc ${automoc_dependencies} ${manual_dependencies}
        COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::moc
            -o ${metatypes_file_gen}
            --collect-json ${automoc_args} ${manual_args}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${metatypes_file_gen}
            ${metatypes_file}
        COMMENT "Running moc --collect-json for target ${target}"
        VERBATIM
    )

    # We can't rely on policy CMP0118 since user project controls it
    set(scope_args)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
        set(scope_args TARGET_DIRECTORY ${target})
    endif()
    set_source_files_properties(${metatypes_file_gen} ${metatypes_file}  ${scope_args}
        PROPERTIES GENERATED TRUE
    )

    # We still need to add this file as a source of the target, otherwise the file
    # rule above is not triggered. INTERFACE_SOURCES do not properly register
    # as dependencies to build the current target.
    # TODO: Can we pass ${metatypes_file} instead of ${metatypes_file_gen} as a source?
    # TODO: Do we still need the _gen variant at all?
    target_sources(${target} PRIVATE ${metatypes_file_gen})
    set_source_files_properties(${metatypes_file} ${scope_args}
        PROPERTIES HEADER_FILE_ONLY TRUE
    )

    set_target_properties(${target} PROPERTIES
        INTERFACE_QT_MODULE_HAS_META_TYPES YES
        INTERFACE_QT_META_TYPES_BUILD_FILE "${metatypes_file}"
    )

    # Set up consumption of files via INTERFACE_SOURCES.
    set(consumes_metatypes "$<BOOL:$<TARGET_PROPERTY:QT_CONSUMES_METATYPES>>")
    set(metatypes_file_genex_build
        "$<BUILD_INTERFACE:$<${consumes_metatypes}:${metatypes_file}>>"
    )
    target_sources(${target} INTERFACE ${metatypes_file_genex_build})

    if(arg_OUTPUT_FILES)
        set(${arg_OUTPUT_FILES} "${metatypes_file}" PARENT_SCOPE)
    endif()

    # Check whether the generated json file needs to be installed.
    # Executable metatypes.json files should not be installed. Qt non-prefix builds should also
    # not install the files.
    set(should_install FALSE)

    if(NOT target_type STREQUAL "EXECUTABLE" AND arg___QT_INTERNAL_INSTALL)
        set(should_install TRUE)
    endif()

    # Automatically fill default install args when not specified.
    if(NOT arg___QT_INTERNAL_INSTALL_DIR)
        # INSTALL_ARCHDATADIR is not set when QtBuildInternals is not loaded
        # (when not doing a Qt build). Default to a hardcoded location for user
        # projects (will likely be wrong).
        if(INSTALL_ARCHDATADIR)
            set(install_dir "${INSTALL_ARCHDATADIR}/metatypes")
        else()
            set(install_dir "lib/metatypes")
        endif()
    else()
        set(install_dir "${arg___QT_INTERNAL_INSTALL_DIR}")
    endif()

    if(should_install)
        set(metatypes_file_install_path "${install_dir}/${metatypes_file_name}")
        set(metatypes_file_install_path_genex "$<INSTALL_PREFIX>/${metatypes_file_install_path}")
        set(metatypes_file_genex_install
            "$<INSTALL_INTERFACE:$<${consumes_metatypes}:${metatypes_file_install_path_genex}>>"
        )
        target_sources(${target} INTERFACE ${metatypes_file_genex_install})
        install(FILES "${metatypes_file}" DESTINATION "${install_dir}")
    endif()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_extract_metatypes)
        qt6_extract_metatypes(${ARGV})
        cmake_parse_arguments(PARSE_ARGV 0 arg "" "OUTPUT_FILES" "")
        if(arg_OUTPUT_FILES)
            set(${arg_OUTPUT_FILES} "${${arg_OUTPUT_FILES}}" PARENT_SCOPE)
        endif()
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

    if(MSVC)
        set(extra_rc_flags "-c65001 -DWIN32 -nologo")
    else()
        set(extra_rc_flags)
    endif()

    if (target_rc_file)
        # Use the provided RC file
        target_sources(${target} PRIVATE "${target_rc_file}")
        set_property(SOURCE ${target_rc_file} PROPERTY COMPILE_FLAGS "${extra_rc_flags}")
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
        #
        # However, https://gitlab.kitware.com/cmake/cmake/-/issues/20682 doesn't let us do that
        # in CMake 3.19 and earlier.
        # We can do it in CMake 3.20 and later.
        # And we have to do it with CMake 3.21.0 to avoid a different issue
        # https://gitlab.kitware.com/cmake/cmake/-/issues/22436
        #
        # So use the object lib work around for <= 3.19 and target_sources directly for later
        # versions.
        set(use_obj_lib FALSE)
        set(end_target "${target}")
        if(CMAKE_VERSION VERSION_LESS 3.20)
            set(use_obj_lib TRUE)
            set(end_target "${target}_rc")
            add_library(${target}_rc OBJECT "${output}")
            target_link_libraries(${target} PRIVATE $<TARGET_OBJECTS:${target}_rc>)
        endif()

        set(scope_args)
        if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
            set(scope_args TARGET_DIRECTORY ${end_target})
        endif()

        while(outputs)
            list(POP_FRONT cfgs cfg)
            list(POP_FRONT outputs output)
            set(input "${output}.tmp")
            add_custom_command(OUTPUT "${output}"
                DEPENDS "${input}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${input}" "${output}"
                VERBATIM
            )
            # We can't rely on policy CMP0118 since user project controls it
            set_source_files_properties(${output} ${scope_args} PROPERTIES
                GENERATED TRUE
                COMPILE_FLAGS "${extra_rc_flags}"
            )
            target_sources(${end_target} PRIVATE "$<$<CONFIG:${cfg}>:${output}>")
        endwhile()
    endif()
endfunction()

# Generate Win32 longPathAware RC and Manifest files for a target.
# MSVC needs the manifest file as part of target_sources. MinGW the RC file.
#
function(_qt_internal_generate_longpath_win32_rc_file_and_manifest target)
    set(prohibited_target_types INTERFACE_LIBRARY STATIC_LIBRARY OBJECT_LIBRARY)
    get_target_property(target_type ${target} TYPE)
    if(target_type IN_LIST prohibited_target_types)
        return()
    endif()

    get_target_property(target_binary_dir ${target} BINARY_DIR)

    # Generate manifest
    set(target_mn_filename "${target}_longpath.manifest")
    set(mn_file_output "${target_binary_dir}/${target_mn_filename}")

    set(mn_contents [=[<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
<application  xmlns="urn:schemas-microsoft-com:asm.v3">
    <windowsSettings xmlns:ws2="http://schemas.microsoft.com/SMI/2016/WindowsSettings">
        <ws2:longPathAware>true</ws2:longPathAware>
    </windowsSettings>
</application>
</assembly>]=])
    file(GENERATE OUTPUT "${mn_file_output}" CONTENT "${mn_contents}")

    # Generate RC File
    set(rc_file_output "${target_binary_dir}/${target}_longpath.rc")
    set(rc_contents "1 /* CREATEPROCESS_MANIFEST_RESOURCE_ID */ 24 /* RT_MANIFEST */ ${target_mn_filename}")
    file(GENERATE OUTPUT "${rc_file_output}" CONTENT "${rc_contents}")

    if (MINGW)
        set(outputs "${rc_file_output}")
    endif()
    list(APPEND outputs "${mn_file_output}")

    foreach(output IN LISTS outputs)
        # Needed for CMake versions < 3.19
        set_source_files_properties(${output} PROPERTIES GENERATED TRUE)
        target_sources(${target} PRIVATE "${output}")
    endforeach()
endfunction()

function(__qt_get_relative_resource_path_for_file output_alias file)
    get_property(alias SOURCE ${file} PROPERTY QT_RESOURCE_ALIAS)
    if (NOT alias)
        set(alias "${file}")
    endif()
    set(${output_alias} ${alias} PARENT_SCOPE)
endfunction()

# Performs linking and propagation of the specified objects via the target's usage requirements.
# The objects may be given as generator expression.
#
# Arguments:
# EXTRA_CONDITIONS
#   Conditions to be checked before linking the object files to the end-point executable.
# EXTRA_TARGET_LINK_LIBRARIES_CONDITIONS
#   Conditions for the target_link_libraries call.
# EXTRA_TARGET_SOURCES_CONDITIONS
#   Conditions for the target_sources call.
function(__qt_internal_propagate_object_files target objects)
    set(options "")
    set(single_args "")
    set(extra_conditions_args
        EXTRA_CONDITIONS
        EXTRA_TARGET_LINK_LIBRARIES_CONDITIONS
        EXTRA_TARGET_SOURCES_CONDITIONS
    )
    set(multi_args ${extra_conditions_args})
    cmake_parse_arguments(arg "${options}" "${single_args}" "${multi_args}" ${ARGN})

    # Collect additional conditions.
    foreach(arg IN LISTS extra_conditions_args)
        string(TOLOWER "${arg}" lcvar)
        if(arg_${arg})
            list(JOIN arg_${arg} "," ${lcvar})
        else()
            set(${lcvar} "$<BOOL:TRUE>")
        endif()
    endforeach()

    # Do not litter the static libraries
    set(not_static_condition
        "$<NOT:$<STREQUAL:$<TARGET_PROPERTY:TYPE>,STATIC_LIBRARY>>"
    )

    # Check if link order matters for the Platform.
    set(platform_link_order_property
        "$<TARGET_PROPERTY:${QT_CMAKE_EXPORT_NAMESPACE}::Platform,_qt_link_order_matters>"
    )
    set(platform_link_order_condition
        "$<BOOL:${platform_link_order_property}>"
    )

    # Check if link options are propagated according to CMP0099
    # In user builds the _qt_cmp0099_policy_check is set to FALSE or $<TARGET_POLICY:CMP0099>
    # depending on the used CMake version.
    # See __qt_internal_check_cmp0099_available for details.
    set(cmp0099_policy_check_property
        "$<TARGET_PROPERTY:${QT_CMAKE_EXPORT_NAMESPACE}::Platform,_qt_cmp0099_policy_check>"
    )
    set(link_objects_using_link_options_condition
        "$<BOOL:$<GENEX_EVAL:${cmp0099_policy_check_property}>>"
    )

    # Collect link conditions for the target_sources call.
    string(JOIN "" target_sources_genex
        "$<"
            "$<AND:"
                "${not_static_condition},"
                "${platform_link_order_condition},"
                "$<NOT:${link_objects_using_link_options_condition}>,"
                "${extra_target_sources_conditions},"
                "${extra_conditions}"
            ">"
        ":${objects}>"
    )
    target_sources(${target} INTERFACE
        "${target_sources_genex}"
    )

    # Collect link conditions for the target_link_options call.
    string(JOIN "" target_link_options_genex
        "$<"
            "$<AND:"
                "${not_static_condition},"
                "${platform_link_order_condition},"
                "${link_objects_using_link_options_condition},"
                "${extra_conditions}"
            ">"
        ":${objects}>"
    )
    # target_link_options works well since CMake 3.17 which has policy CMP0099 set to NEW for the
    # minimum required CMake version greater than or equal to 3.17. The default is OLD. See
    # https://cmake.org/cmake/help/git-master/policy/CMP0099.html for details.
    # This provides yet another way of linking object libraries if user sets the policy to NEW
    # before calling find_package(Qt...).
    target_link_options(${target} INTERFACE
        "${target_link_options_genex}"
    )

    # Collect link conditions for the target_link_libraries call.
    string(JOIN "" target_link_libraries_genex
        "$<"
            "$<AND:"
                "${not_static_condition},"
                "$<NOT:${platform_link_order_condition}>,"
                "${extra_target_link_libraries_conditions},"
                "${extra_conditions}"
            ">"
        ":${objects}>"
    )
    target_link_libraries(${target} INTERFACE
        "${target_link_libraries_genex}"
    )
endfunction()

# Performs linking and propagation of the object library via the target's usage requirements.
# Arguments:
# NO_LINK_OBJECT_LIBRARY_REQUIREMENTS_TO_TARGET skip linking of ${object_library} to ${target}, only
#   propagate $<TARGET_OBJECTS:${object_library}> by linking it to ${target}. It's useful in case
#   if ${object_library} depends on the ${target}. E.g. resource libraries depend on the Core
#   library so linking them back to Core will cause a CMake error.
#
# EXTRA_CONDITIONS object library specific conditions to be checked before link the object library
#   to the end-point executable.
function(__qt_internal_propagate_object_library target object_library)
    set(options NO_LINK_OBJECT_LIBRARY_REQUIREMENTS_TO_TARGET)
    set(single_args "")
    set(multi_args EXTRA_CONDITIONS)
    cmake_parse_arguments(arg "${options}" "${single_args}" "${multi_args}" ${ARGN})

    get_target_property(is_imported ${object_library} IMPORTED)
    if(NOT is_imported)
        target_link_libraries(${object_library} PRIVATE ${QT_CMAKE_EXPORT_NAMESPACE}::Platform)
        _qt_internal_copy_dependency_properties(${object_library} ${target} PRIVATE_ONLY)
    endif()

    # After internal discussion we decided to not rely on the linker order that CMake
    # offers, until CMake provides the guaranteed linking order that suites our needs in a
    # future CMake version.
    # All object libraries mark themselves with the _is_qt_propagated_object_library property.
    # Using a finalizer approach we walk through the target dependencies and look for libraries
    # using the _is_qt_propagated_object_library property. Then, objects of the collected libraries
    # are moved to the beginning of the linker line using target_sources.
    #
    # Note: target_link_libraries works well with linkers other than ld. If user didn't enforce
    # a finalizer we rely on linker to resolve circular dependencies between objects and static
    # libraries.
    set_property(TARGET ${object_library} PROPERTY _is_qt_propagated_object_library TRUE)
    if(NOT is_imported)
        set_property(TARGET ${object_library} APPEND PROPERTY
            EXPORT_PROPERTIES _is_qt_propagated_object_library
        )
    endif()

    # Keep the implicit linking if finalizers are not used.
    set(not_finalizer_mode_condition
        "$<NOT:$<BOOL:$<TARGET_PROPERTY:_qt_object_libraries_finalizer_mode>>>"
    )

    # Use TARGET_NAME to have the correct namespaced name in the exports.
    set(objects "$<TARGET_OBJECTS:$<TARGET_NAME:${object_library}>>")

    __qt_internal_propagate_object_files(${target} ${objects}
        EXTRA_CONDITIONS ${arg_EXTRA_CONDITIONS}
        EXTRA_TARGET_SOURCES_CONDITIONS ${not_finalizer_mode_condition}
        EXTRA_TARGET_LINK_LIBRARIES_CONDITIONS ${not_finalizer_mode_condition}
    )

    if(NOT arg_NO_LINK_OBJECT_LIBRARY_REQUIREMENTS_TO_TARGET)
        # It's necessary to link the object library target, since we want to pass the object library
        # dependencies to the 'target'. Interface linking doesn't add the objects of the library to
        # the end-point linker line but propagates all the dependencies of the object_library added
        # before or AFTER the line below.
        target_link_libraries(${target} INTERFACE ${object_library})
    endif()
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
        target_compile_definitions("${resource_target}" PRIVATE
            "$<TARGET_PROPERTY:${QT_CMAKE_EXPORT_NAMESPACE}::Core,INTERFACE_COMPILE_DEFINITIONS>"
        )
        _qt_internal_set_up_static_runtime_library("${resource_target}")

        # Special handling is required for the Core library resources. The linking of the Core
        # library to the resources adds a circular dependency. This leads to the wrong
        # objects/library order in the linker command line, since the Core library target is
        # resolved first.
        if(NOT target STREQUAL "Core")
            target_link_libraries(${resource_target} INTERFACE ${QT_CMAKE_EXPORT_NAMESPACE}::Core)
        endif()
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

        if(target STREQUAL "Core")
            set(skip_direct_linking NO_LINK_OBJECT_LIBRARY_REQUIREMENTS_TO_TARGET)
        endif()
        __qt_internal_propagate_object_library(${target} ${resource_target}
            ${skip_direct_linking}
        )

        set(${output_generated_target} "${resource_target}" PARENT_SCOPE)
    else()
        set(${output_generated_target} "" PARENT_SCOPE)
        target_sources(${target} PRIVATE ${generated_source_code})
    endif()
endfunction()

# Make file visible in IDEs.
# Targets that are finalized add the file as HEADER_FILE_ONLY in the finalizer.
# Targets that are not finalized add the file under a ${target}_other_files target.
function(_qt_internal_expose_source_file_to_ide target file)
    get_target_property(target_expects_finalization ${target} _qt_expects_finalization)
    if(target_expects_finalization AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
        set_property(TARGET ${target} APPEND PROPERTY _qt_deferred_files ${file})
        return()
    endif()

    # Fallback for targets that are not finalized: Create fake target under which the file is added.
    set(ide_target ${target}_other_files)
    if(NOT TARGET ${ide_target})
        add_custom_target(${ide_target} SOURCES "${file}")

        # The new Xcode build system requires a common target to drive the generation of files,
        # otherwise project configuration fails.
        # By adding ${target} as a dependency of ${target}_other_files,
        # it becomes the common target, so project configuration succeeds.
        if(CMAKE_GENERATOR STREQUAL "Xcode")
            add_dependencies(${ide_target} ${target})
        endif()
    else()
        set_property(TARGET ${ide_target} APPEND PROPERTY SOURCES "${file}")
    endif()

    set(scope_args)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
        set(scope_args TARGET_DIRECTORY "${target}")
    endif()
    get_source_file_property(
        target_dependency "${file}" ${scope_args} _qt_resource_target_dependency)
    if(target_dependency)
        if(NOT TARGET "${target_dependency}")
            message(FATAL_ERROR "Target dependency on source file ${file} is not a cmake target.")
        endif()
        add_dependencies(${ide_target} ${target_dependency})
    endif()
endfunction()

# Called by the target finalizer.
# Adds the files that were added to _qt_deferred_files to SOURCES.
# Sets HEADER_FILES_ONLY if they did not exist yet in SOURCES.
function(_qt_internal_expose_deferred_files_to_ide target)
    get_target_property(new_sources ${target} _qt_deferred_files)
    if(NOT new_sources)
        return()
    endif()
    set(new_sources_real "")
    foreach(f IN LISTS new_sources)
        get_filename_component(realf "${f}" REALPATH)
        list(APPEND new_sources_real ${realf})
    endforeach()

    set(filtered_new_sources "")
    get_target_property(target_source_dir ${target} SOURCE_DIR)
    get_filename_component(target_source_dir "${target_source_dir}" REALPATH)
    get_target_property(existing_sources ${target} SOURCES)
    if(existing_sources)
        set(existing_sources_real "")
        set(realf "")
        foreach(f IN LISTS existing_sources)
            get_filename_component(realf "${f}" REALPATH BASE_DIR ${target_source_dir})
            list(APPEND existing_sources_real ${realf})
        endforeach()

        list(LENGTH new_sources max_i)
        math(EXPR max_i "${max_i} - 1")
        foreach(i RANGE 0 ${max_i})
            list(GET new_sources_real ${i} realf)
            if(NOT realf IN_LIST existing_sources_real)
                list(GET new_sources ${i} f)
                list(APPEND filtered_new_sources ${f})
            endif()
        endforeach()
    endif()
    if("${filtered_new_sources}" STREQUAL "")
        return()
    endif()

    target_sources(${target} PRIVATE ${filtered_new_sources})
    set(scope_args)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
        set(scope_args TARGET_DIRECTORY "${target}")
    endif()
    set_source_files_properties(${filtered_new_sources}
        ${scope_args} PROPERTIES HEADER_FILE_ONLY ON)
endfunction()

#
# Process resources via file path instead of QRC files. Behind the
# scenes, it will generate a qrc file.
#
# The QRC Prefix is set via the PREFIX parameter.
#
# Alias settings for files need to be set via the QT_RESOURCE_ALIAS property
# via the set_source_files_properties() command.
#
# When using this command with static libraries, one or more special targets
# will be generated. Should you wish to perform additional processing on these
# targets pass a value to the OUTPUT_TARGETS parameter.
#
function(_qt_internal_process_resource target resourceName)
    cmake_parse_arguments(rcc "BIG_RESOURCES"
        "PREFIX;LANG;BASE;OUTPUT_TARGETS;DESTINATION" "FILES;OPTIONS" ${ARGN})

    if("${rcc_OPTIONS}" MATCHES "-binary")
        set(isBinary TRUE)
        if(rcc_BIG_RESOURCES)
            message(FATAL_ERROR "BIG_RESOURCES cannot be used together with the -binary option.")
        endif()
    endif()

    if(rcc_BIG_RESOURCES AND CMAKE_GENERATOR STREQUAL "Xcode" AND IOS)
        message(WARNING
            "Due to CMake limitations, the BIG_RESOURCES option can't be used when building "
            "for iOS. "
            "See https://bugreports.qt.io/browse/QTBUG-103497 for details. "
            "Falling back to using regular resources. "
        )
        set(rcc_BIG_RESOURCES OFF)
    endif()

    if(rcc_BIG_RESOURCES AND CMAKE_VERSION VERSION_LESS "3.17")
        message(WARNING
            "The BIG_RESOURCES option does not work reliably with CMake < 3.17. "
            "Consider upgrading to a more recent CMake version or disable the BIG_RESOURCES "
            "option for older CMake versions."
        )
    endif()

    string(REPLACE "/" "_" resourceName ${resourceName})
    string(REPLACE "." "_" resourceName ${resourceName})

    set(resource_files "")
    # Strip the ending slashes from the file_path. If paths contain slashes in the end
    # set/get source properties works incorrect and may have the same QT_RESOURCE_ALIAS
    # for two different paths. See https://gitlab.kitware.com/cmake/cmake/-/issues/23212
    # for details.
    foreach(file_path IN LISTS rcc_FILES)
        if(file_path MATCHES "(.+)/$")
            set(file_path "${CMAKE_MATCH_1}")
        endif()
        list(APPEND resource_files ${file_path})
    endforeach()

    if(NOT "${rcc_BASE}" STREQUAL "")
        get_filename_component(abs_base "${rcc_BASE}" ABSOLUTE)
        foreach(file_path IN LISTS resource_files)
            get_source_file_property(alias "${file_path}" QT_RESOURCE_ALIAS)
            if(alias STREQUAL "NOTFOUND")
                get_filename_component(abs_file "${file_path}" ABSOLUTE)
                file(RELATIVE_PATH rel_file "${abs_base}" "${abs_file}")
                set_property(SOURCE "${file_path}" PROPERTY QT_RESOURCE_ALIAS "${rel_file}")
            endif()
        endforeach()
    endif()

    if(ANDROID)
        if(COMMAND _qt_internal_collect_qml_root_paths)
            _qt_internal_collect_qml_root_paths(${target} ${resource_files})
        endif()
    endif()

    if(NOT rcc_PREFIX)
        get_target_property(rcc_PREFIX ${target} QT_RESOURCE_PREFIX)
        if (NOT rcc_PREFIX)
            set(rcc_PREFIX "/")
        endif()
    endif()

    if (NOT resource_files)
        if (rcc_OUTPUT_TARGETS)
            set(${rcc_OUTPUT_TARGETS} "" PARENT_SCOPE)
        endif()
        return()
    endif()
    set(generatedResourceFile "${CMAKE_CURRENT_BINARY_DIR}/.rcc/${resourceName}.qrc")

    # Generate .qrc file:

    # <RCC><qresource ...>
    set(qrcContents "<RCC>\n  <qresource")
    string(APPEND qrcContents " prefix=\"${rcc_PREFIX}\"")

    if (rcc_LANG)
        string(APPEND qrcContents " lang=\"${rcc_LANG}\"")
    endif()
    string(APPEND qrcContents ">\n")

    set(resource_dependencies)
    foreach(file IN LISTS resource_files)
        __qt_get_relative_resource_path_for_file(file_resource_path ${file})

        if (NOT IS_ABSOLUTE ${file})
            set(file "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
        endif()

        ### FIXME: escape file paths to be XML conform
        # <file ...>...</file>
        string(APPEND qrcContents "    <file alias=\"${file_resource_path}\">")
        string(APPEND qrcContents "${file}</file>\n")
        list(APPEND files "${file}")

        set(scope_args)
        if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
            set(scope_args TARGET_DIRECTORY ${target})
        endif()
        get_source_file_property(
            target_dependency ${file} ${scope_args} _qt_resource_target_dependency)

        # The target dependency code path does not take care of rebuilds when ${file}
        # is touched. Limit its usage to the Xcode generator to avoid the Xcode common
        # dependency issue.
        # TODO: Figure out how to avoid the issue on Xcode, while also enabling proper
        # dependency tracking when ${file} is touched.
        if(target_dependency AND CMAKE_GENERATOR STREQUAL "Xcode")
            if(NOT TARGET ${target_dependency})
                message(FATAL_ERROR
                        "Target dependency on resource file ${file} is not a cmake target.")
            endif()
            list(APPEND resource_dependencies ${target_dependency})
        else()
            list(APPEND resource_dependencies ${file})
        endif()
        _qt_internal_expose_source_file_to_ide(${target} "${file}")
    endforeach()

    # </qresource></RCC>
    string(APPEND qrcContents "  </qresource>\n</RCC>\n")

    set(template_file "${__qt_core_macros_module_base_dir}/Qt6CoreConfigureFileTemplate.in")
    set(qt_core_configure_file_contents "${qrcContents}")
    configure_file("${template_file}" "${generatedResourceFile}")

    set(rccArgs --name "${resourceName}" "${generatedResourceFile}")
    set(rccArgsAllPasses "")

    if(rcc_OPTIONS)
        list(APPEND rccArgsAllPasses ${rcc_OPTIONS})
    endif()

    # When cross-building, we use host tools to generate target code. If the host rcc was compiled
    # with zstd support, it expects the target QtCore to be able to decompress zstd compressed
    # content. This might be true with qmake where host tools are built as part of the
    # cross-compiled Qt, but with CMake we build tools separate from the cross-compiled Qt.
    # If the target does not support zstd (feature is disabled), tell rcc not to generate
    # zstd related code.
    if(NOT QT_FEATURE_zstd)
        list(APPEND rccArgsAllPasses "--no-zstd")
    endif()

    set_property(SOURCE "${generatedResourceFile}" PROPERTY SKIP_AUTOGEN ON)

    # Set output file name for rcc command
    if(isBinary)
        set(generatedOutfile "${CMAKE_CURRENT_BINARY_DIR}/${resourceName}.rcc")
        if(rcc_DESTINATION)
            # Add .rcc suffix if it's not specified by user
            get_filename_component(destinationRccExt "${rcc_DESTINATION}" LAST_EXT)
            if("${destinationRccExt}" STREQUAL ".rcc")
                set(generatedOutfile "${rcc_DESTINATION}")
            else()
                set(generatedOutfile "${rcc_DESTINATION}.rcc")
            endif()
        endif()
    elseif(rcc_BIG_RESOURCES)
        set(generatedOutfile "${CMAKE_CURRENT_BINARY_DIR}/.rcc/qrc_${resourceName}_tmp.cpp")
    else()
        set(generatedOutfile "${CMAKE_CURRENT_BINARY_DIR}/.rcc/qrc_${resourceName}.cpp")
    endif()

    set(pass_msg)
    if(rcc_BIG_RESOURCES)
        list(PREPEND rccArgs --pass 1)
        set(pass_msg " pass 1")
    endif()

    list(PREPEND rccArgs --output "${generatedOutfile}")

    # Process .qrc file:
    add_custom_command(OUTPUT "${generatedOutfile}"
                       COMMAND "${QT_CMAKE_EXPORT_NAMESPACE}::rcc" ${rccArgs} ${rccArgsAllPasses}
                       DEPENDS
                        ${resource_dependencies}
                        ${generatedResourceFile}
                        "${QT_CMAKE_EXPORT_NAMESPACE}::rcc"
                       COMMENT "Running rcc${pass_msg} for resource ${resourceName}"
                       VERBATIM)

    if(isBinary)
        # Add generated .rcc target to 'all' set
        add_custom_target(binary_resource_${resourceName} ALL DEPENDS "${generatedOutfile}")
        return()
    endif()

    # We can't rely on policy CMP0118 since user project controls it.
    # We also want SKIP_AUTOGEN known in the target's scope, where we can.
    set(scope_args)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
        set(scope_args TARGET_DIRECTORY ${target})
    endif()
    set_source_files_properties(${generatedOutfile} ${scope_args} PROPERTIES
        SKIP_AUTOGEN TRUE
        GENERATED TRUE
        SKIP_UNITY_BUILD_INCLUSION TRUE
        SKIP_PRECOMPILE_HEADERS TRUE
    )

    get_target_property(target_source_dir ${target} SOURCE_DIR)
    if(NOT target_source_dir STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        # We have to create a separate target in this scope that depends on
        # the generated file, otherwise the original target won't have the
        # required dependencies in place to ensure correct build order.
        add_custom_target(${target}_${resourceName} DEPENDS ${generatedOutfile})
        add_dependencies(${target} ${target}_${resourceName})
    endif()

    if(rcc_BIG_RESOURCES)
        set(pass1OutputFile ${generatedOutfile})
        set(generatedOutfile
            "${CMAKE_CURRENT_BINARY_DIR}/.rcc/qrc_${resourceName}${CMAKE_CXX_OUTPUT_EXTENSION}")
        _qt_internal_add_rcc_pass2(
            RESOURCE_NAME ${resourceName}
            RCC_OPTIONS ${rccArgsAllPasses}
            OBJECT_LIB ${target}_${resourceName}_obj
            QRC_FILE ${generatedResourceFile}
            PASS1_OUTPUT_FILE ${pass1OutputFile}
            OUT_OBJECT_FILE ${generatedOutfile}
        )
        get_target_property(type ${target} TYPE)
        if(type STREQUAL STATIC_LIBRARY)
            # Create a custom target to trigger the generation of ${generatedOutfile}
            set(pass2_target ${target}_${resourceName}_pass2)
            add_custom_target(${pass2_target} DEPENDS ${generatedOutfile})
            add_dependencies(${target} ${pass2_target})

            # Propagate the object files to the target.
            __qt_internal_propagate_object_files(${target} "${generatedOutfile}")
        else()
            target_sources(${target} PRIVATE ${generatedOutfile})
        endif()
    else()
        __qt_propagate_generated_resource(${target} ${resourceName} "${generatedOutfile}"
            output_targets)
    endif()

    set_property(TARGET ${target}
        APPEND PROPERTY _qt_generated_qrc_files "${generatedResourceFile}")

    if (rcc_OUTPUT_TARGETS)
        set(${rcc_OUTPUT_TARGETS} "${output_targets}" PARENT_SCOPE)
    endif()
endfunction()

macro(_qt_internal_get_add_plugin_keywords option_args single_args multi_args)
    set(${option_args}
        STATIC
        SHARED
        __QT_INTERNAL_NO_PROPAGATE_PLUGIN_INITIALIZER
    )
    set(${single_args}
        PLUGIN_TYPE   # Internal use only, may be changed or removed
        CLASS_NAME
        OUTPUT_NAME   # Internal use only, may be changed or removed
        OUTPUT_TARGETS
    )
    set(${multi_args})
endmacro()

function(qt6_add_plugin target)
    _qt_internal_get_add_plugin_keywords(opt_args single_args multi_args)
    list(APPEND opt_args MANUAL_FINALIZATION)

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")

    if(arg_STATIC AND arg_SHARED)
        message(FATAL_ERROR
            "Both STATIC and SHARED options were given. Only one of the two should be used."
        )
    endif()

    # Explicit option takes priority over the computed default.
    if(arg_STATIC)
        set(create_static_plugin TRUE)
    elseif(arg_SHARED)
        set(create_static_plugin FALSE)
    else()
        # If no explicit STATIC/SHARED option is set, default to the flavor of the Qt build.
        if(QT6_IS_SHARED_LIBS_BUILD)
            set(create_static_plugin FALSE)
        else()
            set(create_static_plugin TRUE)
        endif()
    endif()

    # The default of _qt_internal_add_library creates SHARED in a shared Qt build, so we need to
    # be explicit about the MODULE.
    if(create_static_plugin)
        set(type_to_create STATIC)
    else()
        set(type_to_create MODULE)
    endif()

    _qt_internal_add_library(${target} ${type_to_create} ${arg_UNPARSED_ARGUMENTS})
    set_property(TARGET ${target} PROPERTY _qt_expects_finalization TRUE)

    get_target_property(target_type "${target}" TYPE)
    if (target_type STREQUAL "STATIC_LIBRARY")
        target_compile_definitions(${target} PRIVATE QT_STATICPLUGIN)
    endif()

    set(output_name ${target})
    if (arg_OUTPUT_NAME)
        set(output_name ${arg_OUTPUT_NAME})
    endif()
    set_property(TARGET "${target}" PROPERTY OUTPUT_NAME "${output_name}")

    if (ANDROID)
        set_target_properties(${target}
            PROPERTIES
            LIBRARY_OUTPUT_NAME "plugins_${arg_PLUGIN_TYPE}_${output_name}"
        )
    endif()

    # Derive the class name from the target name if it's not explicitly specified.
    set(plugin_class_name "")
    if (NOT "${arg_PLUGIN_TYPE}" STREQUAL "qml_plugin")
        if (NOT arg_CLASS_NAME)
            set(plugin_class_name "${target}")
        else()
            set(plugin_class_name "${arg_CLASS_NAME}")
        endif()
    else()
        # Make sure to set any passed-in class name for qml plugins as well, because it's used for
        # building the qml plugin foo_init object libraries.
        if(arg_CLASS_NAME)
            set(plugin_class_name "${arg_CLASS_NAME}")
        else()
            message(FATAL_ERROR "Qml plugin target has no CLASS_NAME specified: '${target}'")
        endif()
    endif()

    set_target_properties(${target} PROPERTIES QT_PLUGIN_CLASS_NAME "${plugin_class_name}")

    # Create a plugin initializer object library for static plugins.
    # It contains a Q_IMPORT_PLUGIN(QT_PLUGIN_CLASS_NAME) call.
    # Project targets will automatically link to the plugin initializer whenever they link to the
    # plugin target.
    # The plugin init target name is stored in OUTPUT_TARGETS, so projects may install them.
    # Qml plugin inits are handled in Qt6QmlMacros.
    if(NOT "${arg_PLUGIN_TYPE}" STREQUAL "qml_plugin"
            AND target_type STREQUAL "STATIC_LIBRARY")
        __qt_internal_add_static_plugin_init_object_library("${target}" plugin_init_target)

        if(arg_OUTPUT_TARGETS)
            set(${arg_OUTPUT_TARGETS} ${plugin_init_target} PARENT_SCOPE)
        endif()

        # We don't automatically propagate the plugin init library for Qt provided plugins, because
        # there are 2 other code paths that take care of that, one involving finalizers and the
        # other regular usage requirements.
        if(NOT arg___QT_INTERNAL_NO_PROPAGATE_PLUGIN_INITIALIZER)
            __qt_internal_propagate_object_library("${target}" "${plugin_init_target}")
        endif()
    else()
        if(arg_OUTPUT_TARGETS)
            set(${arg_OUTPUT_TARGETS} "" PARENT_SCOPE)
        endif()
    endif()

    target_compile_definitions(${target} PRIVATE
        QT_PLUGIN
        QT_DEPRECATED_WARNINGS
    )

    if(target_type STREQUAL "MODULE_LIBRARY")
        if(NOT TARGET qt_internal_plugins)
            add_custom_target(qt_internal_plugins)
            _qt_internal_assign_to_internal_targets_folder(qt_internal_plugins)
        endif()
        add_dependencies(qt_internal_plugins ${target})
    endif()

    if(arg_MANUAL_FINALIZATION)
        # Caller says they will call qt6_finalize_target() themselves later
        return()
    endif()

    # Defer the finalization if we can. When the caller's project requires
    # CMake 3.19 or later, this makes the calls to this function concise while
    # still allowing target property modification before finalization.
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
        # Need to wrap in an EVAL CODE or else ${target} won't be evaluated
        # due to special behavior of cmake_language() argument handling
        cmake_language(EVAL CODE "cmake_language(DEFER CALL qt6_finalize_target ${target})")
    else()
        set_target_properties("${target}" PROPERTIES _qt_is_immediately_finalized TRUE)
        qt6_finalize_target("${target}")
    endif()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_plugin)
        qt6_add_plugin(${ARGV})
        cmake_parse_arguments(PARSE_ARGV 1 arg "" "OUTPUT_TARGETS" "")
        if(arg_OUTPUT_TARGETS)
            set(${arg_OUTPUT_TARGETS} ${${arg_OUTPUT_TARGETS}} PARENT_SCOPE)
        endif()
    endfunction()
endif()

# Creates a library by forwarding arguments to add_library, applies some Qt naming file name naming
# conventions and ensures the execution of Qt specific finalizers.
function(qt6_add_library target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "MANUAL_FINALIZATION" "" "")

    _qt_internal_add_library("${target}" ${arg_UNPARSED_ARGUMENTS})
    set_property(TARGET ${target} PROPERTY _qt_expects_finalization TRUE)

    if(arg_MANUAL_FINALIZATION)
        # Caller says they will call qt6_finalize_target() themselves later
        return()
    endif()

    # Defer the finalization if we can. When the caller's project requires
    # CMake 3.19 or later, this makes the calls to this function concise while
    # still allowing target property modification before finalization.
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
        # Need to wrap in an EVAL CODE or else ${target} won't be evaluated
        # due to special behavior of cmake_language() argument handling
        cmake_language(EVAL CODE "cmake_language(DEFER CALL qt6_finalize_target ${target})")
    else()
        set_target_properties("${target}" PROPERTIES _qt_is_immediately_finalized TRUE)
        qt6_finalize_target("${target}")
    endif()
endfunction()

# Creates a library target by forwarding the arguments to add_library.
#
# Applies some Qt specific behaviors:
# - If no type option is specified, rather than defaulting to STATIC it defaults to STATIC or SHARED
#   depending on the Qt configuration.
# - Applies Qt specific prefixes and suffixes to file names depending on platform.
function(_qt_internal_add_library target)
    set(opt_args
        STATIC
        SHARED
        MODULE
        INTERFACE
        OBJECT
    )
    set(single_args "")
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")

    set(option_type_count 0)
    if(arg_STATIC)
        set(type_to_create STATIC)
        math(EXPR option_type_count "${option_type_count}+1")
    elseif(arg_SHARED)
        set(type_to_create SHARED)
        math(EXPR option_type_count "${option_type_count}+1")
    elseif(arg_MODULE)
        set(type_to_create MODULE)
        math(EXPR option_type_count "${option_type_count}+1")
    elseif(arg_INTERFACE)
        set(type_to_create INTERFACE)
        math(EXPR option_type_count "${option_type_count}+1")
    elseif(arg_OBJECT)
        set(type_to_create OBJECT)
        math(EXPR option_type_count "${option_type_count}+1")
    endif()

    if(option_type_count GREATER 1)
        message(FATAL_ERROR
            "Multiple type options were given. Only one should be used."
        )
    endif()

    # If no explicit type option is set, default to the flavor of the Qt build.
    # This in contrast to CMake which defaults to STATIC.
    if(NOT arg_STATIC AND NOT arg_SHARED AND NOT arg_MODULE AND NOT arg_INTERFACE
            AND NOT arg_OBJECT)
        if(QT6_IS_SHARED_LIBS_BUILD)
            set(type_to_create SHARED)
        else()
            set(type_to_create STATIC)
        endif()
    endif()

    add_library(${target} ${type_to_create} ${arg_UNPARSED_ARGUMENTS})
    _qt_internal_set_up_static_runtime_library(${target})

    if(NOT type_to_create STREQUAL "INTERFACE" AND NOT type_to_create STREQUAL "OBJECT")
        _qt_internal_apply_win_prefix_and_suffix("${target}")
    endif()

    if(arg_MODULE AND APPLE)
        # CMake defaults to using .so extensions for loadable modules, aka plugins,
        # but Qt plugins are actually suffixed with .dylib.
        set_property(TARGET "${target}" PROPERTY SUFFIX ".dylib")
    endif()

    if(ANDROID)
        set_property(TARGET "${target}"
                     PROPERTY _qt_android_apply_arch_suffix_called_from_qt_impl TRUE)
        qt6_android_apply_arch_suffix("${target}")
    endif()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_library)
        qt6_add_library(${ARGV})
    endfunction()
endif()

# TODO: Remove once all repositories use qt_internal_add_example instead of add_subdirectory.
macro(_qt_internal_override_example_install_dir_to_dot)
    # Set INSTALL_EXAMPLEDIR to ".".
    # This overrides the install destination of unclean Qt example projects to install directly
    # to CMAKE_INSTALL_PREFIX.
    if(QT_INTERNAL_SET_EXAMPLE_INSTALL_DIR_TO_DOT)
        set(INSTALL_EXAMPLEDIR ".")
        set(_qt_internal_example_dir_set_to_dot TRUE)
    endif()
endmacro()

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

# Copies properties of the dependency to the target.
# Arguments:
#   PROPERTIES list of properties to copy. If not specified the following properties are copied
#              by default: INCLUDE_DIRECTORIES SYSTEM_INCLUDE_DIRECTORIES COMPILE_DEFINITIONS
#              COMPILE_OPTIONS COMPILE_FEATURES
#   PRIVATE_ONLY copy only private properties (without INTERFACE analogues). Optional.
#   INTERFACE_ONLY copy only interface properties (without non-prefixed analogues). Optional.
#      Note: Not all properties have INTERFACE properties analogues.
#            See https://cmake.org/cmake/help/latest/prop_tgt/EXPORT_PROPERTIES.html for details.
#
# PRIVATE_ONLY and INTERFACE_ONLY in the same call are not allowed. Omit these options to copy
# both sets.
function(_qt_internal_copy_dependency_properties target dependency)
    cmake_parse_arguments(arg "INTERFACE_ONLY;PRIVATE_ONLY" "" "PROPERTIES" ${ARGN})
    if(arg_PRIVATE_ONLY AND arg_INTERFACE_ONLY)
        message("Both PRIVATE_ONLY and INTERFACE_ONLY options are set.\
Please use _qt_internal_copy_dependency_properties without these options to copy a set of
properties of both types."
        )
    endif()

    if(arg_PROPERTIES)
        set(common_props_to_set ${arg_PROPERTIES})
    else()
        set(common_props_to_set
            INCLUDE_DIRECTORIES SYSTEM_INCLUDE_DIRECTORIES
            COMPILE_DEFINITIONS COMPILE_OPTIONS
            COMPILE_FEATURES
        )
    endif()

    set(props_to_set "")
    if(NOT arg_INTERFACE_ONLY)
        set(props_to_set ${common_props_to_set})
    endif()
    if(NOT arg_PRIVATE_ONLY)
        list(TRANSFORM common_props_to_set PREPEND INTERFACE_
                        OUTPUT_VARIABLE interface_properties)
        list(APPEND props_to_set ${interface_properties})
    endif()

    foreach(prop ${props_to_set})
        set_property(TARGET
            "${target}" APPEND PROPERTY
            ${prop} "$<TARGET_PROPERTY:${dependency},${prop}>"
        )
    endforeach()
endfunction()

function(qt6_disable_unicode_defines target)
    set_target_properties(${target} PROPERTIES QT_NO_UNICODE_DEFINES TRUE)
endfunction()

# Finalizer function for the top-level user projects.
#
# This function is currently in Technical Preview.
# Its signature and behavior might change.
function(qt6_finalize_project)
    if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        message("qt6_finalize_project is called not in the top-level CMakeLists.txt.")
    endif()
    if(ANDROID)
        _qt_internal_collect_apk_dependencies()
    endif()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_finalize_project)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_finalize_project()
        else()
            message(FATAL_ERROR "qt_finalize_project() is only available in Qt 6.")
        endif()
    endfunction()

    function(qt_disable_unicode_defines)
        qt6_disable_unicode_defines(${ARGV})
    endfunction()
endif()

function(_qt_internal_get_deploy_impl_dir var)
    set(${var} "${CMAKE_BINARY_DIR}/.qt" PARENT_SCOPE)
endfunction()

function(_qt_internal_add_deploy_support deploy_support_file)
    get_filename_component(deploy_support_file "${deploy_support_file}" REALPATH)

    set(target ${QT_CMAKE_EXPORT_NAMESPACE}::Core)
    get_target_property(aliased_target ${target} ALIASED_TARGET)
    if(aliased_target)
        set(target ${aliased_target})
    endif()

    get_property(scripts TARGET ${target} PROPERTY _qt_deploy_support_files)
    if(NOT "${deploy_support_file}" IN_LIST scripts)
        set_property(TARGET ${target} APPEND PROPERTY
            _qt_deploy_support_files "${deploy_support_file}"
        )
    endif()
endfunction()

# Sets up the commands for use at install/deploy time
function(_qt_internal_setup_deploy_support)
    if(QT_SKIP_SETUP_DEPLOYMENT)
        return()
    endif()

    get_property(cmake_role GLOBAL PROPERTY CMAKE_ROLE)
    if(NOT cmake_role STREQUAL "PROJECT")
        return()
    endif()

    # Always set QT_DEPLOY_SUPPORT in the caller's scope, even if we've generated
    # the deploy support file in a previous call. The project may be calling
    # find_package() from sibling directories with separate variable scopes.
    _qt_internal_get_deploy_impl_dir(deploy_impl_dir)

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        set(QT_DEPLOY_SUPPORT "${deploy_impl_dir}/QtDeploySupport-$<CONFIG>.cmake")
    else()
        set(QT_DEPLOY_SUPPORT "${deploy_impl_dir}/QtDeploySupport.cmake")
    endif()
    set(QT_DEPLOY_SUPPORT "${QT_DEPLOY_SUPPORT}" PARENT_SCOPE)

    get_property(have_generated_file GLOBAL PROPERTY _qt_have_generated_deploy_support)
    if(have_generated_file)
        return()
    endif()
    set_property(GLOBAL PROPERTY _qt_have_generated_deploy_support TRUE)

    include(GNUInstallDirs)
    set(target ${QT_CMAKE_EXPORT_NAMESPACE}::Core)
    get_target_property(aliased_target ${target} ALIASED_TARGET)
    if(aliased_target)
        set(target ${aliased_target})
    endif()

    # Make sure to look under the Qt bin dir with find_program, rather than randomly picking up
    # a deployqt tool in the system.
    # QT6_INSTALL_PREFIX is not set during Qt build, so add the hints conditionally.
    set(find_program_hints)
    if(QT6_INSTALL_PREFIX)
        set(find_program_hints HINTS ${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS})
    endif()

    # In the generator expression logic below, we need safe_target_file because
    # CMake evaluates expressions in both the TRUE and FALSE branches of $<IF:...>.
    # We still need a target to give to $<TARGET_FILE:...> when we have no deploy
    # tool, so we cannot use something like $<TARGET_FILE:macdeployqt> directly.
    if(APPLE AND NOT IOS)
        find_program(MACDEPLOYQT_EXECUTABLE macdeployqt
            ${find_program_hints})
        set(fallback "$<$<BOOL:${MACDEPLOYQT_EXECUTABLE}>:${MACDEPLOYQT_EXECUTABLE}>")
        set(target_if_exists "$<TARGET_NAME_IF_EXISTS:${QT_CMAKE_EXPORT_NAMESPACE}::macdeployqt>")
        set(have_deploy_tool "$<BOOL:${target_if_exists}>")
        set(safe_target_file
            "$<TARGET_FILE:$<IF:${have_deploy_tool},${target_if_exists},${target}>>")
        set(__QT_DEPLOY_TOOL "$<IF:${have_deploy_tool},${safe_target_file},${fallback}>")
    elseif(WIN32)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt
            ${find_program_hints})
        set(fallback "$<$<BOOL:${WINDEPLOYQT_EXECUTABLE}>:${WINDEPLOYQT_EXECUTABLE}>")
        set(target_if_exists "$<TARGET_NAME_IF_EXISTS:${QT_CMAKE_EXPORT_NAMESPACE}::windeployqt>")
        set(have_deploy_tool "$<BOOL:${target_if_exists}>")
        set(safe_target_file
            "$<TARGET_FILE:$<IF:${have_deploy_tool},${target_if_exists},${target}>>")
        set(__QT_DEPLOY_TOOL "$<IF:${have_deploy_tool},${safe_target_file},${fallback}>")
    elseif(UNIX AND NOT APPLE AND NOT ANDROID AND NOT CMAKE_CROSSCOMPILING)
        set(__QT_DEPLOY_TOOL "GRD")
    else()
        # Android is handled as a build target, not via this install-based approach.
        # Therefore, we don't consider androiddeployqt here.
        set(__QT_DEPLOY_TOOL "")
    endif()

    _qt_internal_add_deploy_support("${CMAKE_CURRENT_LIST_DIR}/Qt6CoreDeploySupport.cmake")

    set(deploy_ignored_lib_dirs "")
    if(__QT_DEPLOY_TOOL STREQUAL "GRD" AND NOT "${QT6_INSTALL_PREFIX}" STREQUAL "")
        # Set up the directories we want to ignore when running file(GET_RUNTIME_DEPENDENCIES).
        # If the Qt prefix is the root of one of those directories, don't ignore that directory.
        # For example, if Qt's installation prefix is /usr, then we don't want to ignore /usr/lib.
        foreach(link_dir IN LISTS CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES)
            file(RELATIVE_PATH relative_dir "${QT6_INSTALL_PREFIX}" "${link_dir}")
            if(relative_dir STREQUAL "")
                # The Qt prefix is exactly ${link_dir}.
                continue()
            endif()
            if(IS_ABSOLUTE "${relative_dir}" OR relative_dir MATCHES "^\\.\\./")
                # The Qt prefix is outside of ${link_dir}.
                list(APPEND deploy_ignored_lib_dirs "${link_dir}")
            endif()
        endforeach()
    endif()

    # Check whether we will have to adjust the RPATH of plugins.
    if("${QT_DEPLOY_FORCE_ADJUST_RPATHS}" STREQUAL "")
        set(must_adjust_plugins_rpath "")
        if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows"
                AND NOT CMAKE_INSTALL_LIBDIR STREQUAL QT6_INSTALL_LIBS)
            set(must_adjust_plugins_rpath ON)
        endif()
    else()
        set(must_adjust_plugins_rpath "${QT_DEPLOY_FORCE_ADJUST_RPATHS}")
    endif()

    # Find the patchelf executable if necessary.
    if(must_adjust_plugins_rpath)
        if(CMAKE_VERSION VERSION_LESS "3.21")
            set(QT_DEPLOY_USE_PATCHELF ON)
        endif()
        if(QT_DEPLOY_USE_PATCHELF)
            find_program(QT_DEPLOY_PATCHELF_EXECUTABLE patchelf)
            if(NOT QT_DEPLOY_PATCHELF_EXECUTABLE)
                set(QT_DEPLOY_PATCHELF_EXECUTABLE "patchelf")
                message(WARNING "The patchelf executable could not be located. "
                    "To use Qt's CMake deployment API, install patchelf or upgrade CMake to 3.21 "
                    "or newer.")
            endif()
        endif()
    endif()

    file(GENERATE OUTPUT "${QT_DEPLOY_SUPPORT}" CONTENT
"cmake_minimum_required(VERSION 3.16...3.21)

# These are part of the public API. Projects should use them to provide a
# consistent set of prefix-relative destinations.
if(NOT QT_DEPLOY_BIN_DIR)
    set(QT_DEPLOY_BIN_DIR \"${CMAKE_INSTALL_BINDIR}\")
endif()
if(NOT QT_DEPLOY_LIB_DIR)
    set(QT_DEPLOY_LIB_DIR \"${CMAKE_INSTALL_LIBDIR}\")
endif()
if(NOT QT_DEPLOY_PLUGINS_DIR)
    set(QT_DEPLOY_PLUGINS_DIR \"plugins\")
endif()
if(NOT QT_DEPLOY_QML_DIR)
    set(QT_DEPLOY_QML_DIR \"qml\")
endif()
if(NOT QT_DEPLOY_TRANSLATIONS_DIR)
    set(QT_DEPLOY_TRANSLATIONS_DIR \"translations\")
endif()
if(NOT QT_DEPLOY_PREFIX)
    set(QT_DEPLOY_PREFIX \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}\")
endif()
if(QT_DEPLOY_PREFIX STREQUAL \"\")
    set(QT_DEPLOY_PREFIX .)
endif()
if(NOT QT_DEPLOY_IGNORED_LIB_DIRS)
    set(QT_DEPLOY_IGNORED_LIB_DIRS \"${deploy_ignored_lib_dirs}\")
endif()

# These are internal implementation details. They may be removed at any time.
set(__QT_DEPLOY_SYSTEM_NAME \"${CMAKE_SYSTEM_NAME}\")
set(__QT_DEPLOY_IS_SHARED_LIBS_BUILD \"${QT6_IS_SHARED_LIBS_BUILD}\")
set(__QT_DEPLOY_TOOL \"${__QT_DEPLOY_TOOL}\")
set(__QT_DEPLOY_IMPL_DIR \"${deploy_impl_dir}\")
set(__QT_DEPLOY_VERBOSE \"${QT_ENABLE_VERBOSE_DEPLOYMENT}\")
set(__QT_CMAKE_EXPORT_NAMESPACE \"${QT_CMAKE_EXPORT_NAMESPACE}\")
set(__QT_DEPLOY_GENERATOR_IS_MULTI_CONFIG \"${is_multi_config}\")
set(__QT_DEPLOY_ACTIVE_CONFIG \"$<CONFIG>\")
set(__QT_NO_CREATE_VERSIONLESS_FUNCTIONS \"${QT_NO_CREATE_VERSIONLESS_FUNCTIONS}\")
set(__QT_DEFAULT_MAJOR_VERSION \"${QT_DEFAULT_MAJOR_VERSION}\")
set(__QT_DEPLOY_QT_ADDITIONAL_PACKAGES_PREFIX_PATH \"${QT_ADDITIONAL_PACKAGES_PREFIX_PATH}\")
set(__QT_DEPLOY_QT_INSTALL_PREFIX \"${QT6_INSTALL_PREFIX}\")
set(__QT_DEPLOY_QT_INSTALL_BINS \"${QT6_INSTALL_BINS}\")
set(__QT_DEPLOY_QT_INSTALL_PLUGINS \"${QT6_INSTALL_PLUGINS}\")
set(__QT_DEPLOY_QT_INSTALL_TRANSLATIONS \"${QT6_INSTALL_TRANSLATIONS}\")
set(__QT_DEPLOY_PLUGINS \"\")
set(__QT_DEPLOY_MUST_ADJUST_PLUGINS_RPATH \"${must_adjust_plugins_rpath}\")
set(__QT_DEPLOY_USE_PATCHELF \"${QT_DEPLOY_USE_PATCHELF}\")
set(__QT_DEPLOY_PATCHELF_EXECUTABLE \"${QT_DEPLOY_PATCHELF_EXECUTABLE}\")

# Define the CMake commands to be made available during deployment.
set(__qt_deploy_support_files
    \"$<JOIN:$<TARGET_PROPERTY:${target},_qt_deploy_support_files>,\"
    \">\"
)
foreach(__qt_deploy_support_file IN LISTS __qt_deploy_support_files)
    include(\"\${__qt_deploy_support_file}\")
endforeach()

unset(__qt_deploy_support_file)
unset(__qt_deploy_support_files)
")
endfunction()

# We basically mirror CMake's policy setup
# A policy can be set to OLD, set to NEW or unset
# unset is the default state
#
function(qt6_policy mode policy behaviorOrVariable)
    # When building Qt, tests and examples might expect a policy to be known, but they won't be
    # known depending on which scope or when a find_package(Module) with the respective policy
    # is called. Check the global list of known policies to accommodate that.
    if(QT_BUILDING_QT AND NOT DEFINED QT_KNOWN_POLICY_${policy})
        get_property(global_known_policies GLOBAL PROPERTY _qt_global_known_policies)
        if(policy IN_LIST global_known_policies)
            set(QT_KNOWN_POLICY_${policy} TRUE)
        endif()
    endif()

    if (NOT DEFINED QT_KNOWN_POLICY_${policy})
        message(FATAL_ERROR
            "${policy} is not a known Qt policy. Did you include the necessary Qt module?"
        )
    endif()
    if (${mode} STREQUAL "SET")
        set(behavior ${behaviorOrVariable})
        if (${behavior} STREQUAL "NEW" OR ${behavior} STREQUAL "OLD")
            set(__QT_INTERNAL_POLICY_${policy} ${behavior} PARENT_SCOPE)
        else()
            message(FATAL_ERROR "Qt policies must be either set to NEW or OLD, but got ${behavior}")
        endif()
    else(${mode} STREQUAL "GET")
        set(variable "${behaviorOrVariable}")
        set("${variable}" "${__QT_INTERNAL_POLICY_${policy}}" PARENT_SCOPE)
    endif()
endfunction()

# Internal helper function; can be used in any module before doing a policy check
function(__qt_internal_setup_policy policy sinceversion policyexplanation)
    if(DEFINED __QT_INTERNAL_POLICY_${policy})
        if (__QT_INTERNAL_POLICY_${policy} STREQUAL "OLD")
            # policy is explicitly disabled
            message(DEPRECATION
                "Qt policy ${policy} is set to OLD. "
                "Support for the old behavior will be removed in a future major version of Qt."
            )
        endif()
        #else: policy is already enabled, nothing to do
    elseif (${sinceversion} VERSION_LESS_EQUAL __qt_policy_check_version)
        # we cannot use the public function here as we want to set it in parent scope
        set(__QT_INTERNAL_POLICY_${policy} "NEW" PARENT_SCOPE)
    elseif(NOT "${QT_NO_SHOW_OLD_POLICY_WARNINGS}")
        message(AUTHOR_WARNING
            "Qt policy ${policy} is not set: "
            "${policyexplanation} "
            "Use the qt_policy command to set the policy and suppress this warning.\n"
        )
    endif()
endfunction()

# Note this needs to be a macro because it sets variables intended for the
# calling scope.
macro(qt6_standard_project_setup)
    # A parent project might want to prevent child projects pulled in with
    # add_subdirectory() from changing the parent's preferred arrangement.
    # They can set this variable to true to effectively disable this function.
    if(NOT QT_NO_STANDARD_PROJECT_SETUP)

        set(__qt_sps_args_option)
        set(__qt_sps_args_single
            REQUIRES
            SUPPORTS_UP_TO
        )
        set(__qt_sps_args_multi)
        cmake_parse_arguments(__qt_sps_arg
            "${__qt_sps_args_option}"
            "${__qt_sps_args_single}"
            "${__qt_sps_args_multi}"
            ${ARGN}
        )

        if(__qt_sps_arg_UNPARSED_ARGUMENTS)
            message(FATAL_ERROR "Unexpected arguments: ${arg_UNPARSED_ARGUMENTS}")
        endif()

        # Set the Qt CMake policy based on the requested version(s)
        set(__qt_policy_check_version "6.0.0")
        if(Qt6_VERSION_MAJOR)
            set(__qt_current_version
                "${Qt6_VERSION_MAJOR}.${Qt6_VERSION_MINOR}.${Qt6_VERSION_PATCH}")
        elseif(QT_BUILDING_QT)
            set(__qt_current_version
                "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
        else()
            message(FATAL_ERROR "Can not determine Qt version.")
        endif()
        if(__qt_sps_arg_REQUIRES)
            if("${__qt_current_version}" VERSION_LESS "${__qt_sps_arg_REQUIRES}")
                message(FATAL_ERROR
                    "Project required a Qt minimum version of ${__qt_sps_arg_REQUIRES}, "
                    "but current version is only ${__qt_current_version}.")
            endif()
            set(__qt_policy_check_version "${__qt_sps_arg_REQUIRES}")
        endif()
        if(__qt_sps_arg_SUPPORTS_UP_TO)
            if(__qt_sps_arg_REQUIRES)
                if(${__qt_sps_arg_SUPPORTS_UP_TO} VERSION_LESS ${__qt_sps_arg_REQUIRES})
                    message(FATAL_ERROR "SUPPORTS_UP_TO must be larger than or equal to REQUIRES.")
                endif()
                set(__qt_policy_check_version "${__qt_sps_arg_SUPPORTS_UP_TO}")
            else()
                message(FATAL_ERROR "Please specify the REQUIRES as well.")
            endif()
        endif()

        # All changes below this point should not result in a change to an
        # existing value, except for CMAKE_INSTALL_RPATH which may append new
        # values (but no duplicates).

        # Use standard install locations, provided by GNUInstallDirs. All
        # platforms should have this included so that we know the
        # CMAKE_INSTALL_xxxDIR variables will be set.
        include(GNUInstallDirs)
        if(WIN32)
            # Windows has no RPATH support, so we need all non-plugin DLLs in
            # the same directory as application executables if we want to be
            # able to run them without having to augment the PATH environment
            # variable. Don't discard an existing value in case the project has
            # already set this to somewhere else. Our setting is somewhat
            # opinionated, so make it easy for projects to choose something else.
            if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
                set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
            endif()
        elseif(NOT APPLE)
            # Apart from Windows and Apple, most other platforms support RPATH
            # and $ORIGIN. Make executables and non-static libraries use an
            # install RPATH that allows them to find library dependencies if the
            # project installs things to the directories defined by the
            # CMAKE_INSTALL_xxxDIR variables (which is what CMake's defaults
            # are based on).
            file(RELATIVE_PATH __qt_relDir
                ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}
                ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}
            )
            list(APPEND CMAKE_INSTALL_RPATH $ORIGIN $ORIGIN/${__qt_relDir})
            list(REMOVE_DUPLICATES CMAKE_INSTALL_RPATH)
            unset(__qt_reldir)
        endif()

        # Turn these on by default, unless they are already set. Projects can
        # always turn off any they really don't want after we return.
        foreach(auto_set IN ITEMS MOC UIC)
            if(NOT DEFINED CMAKE_AUTO${auto_set})
                set(CMAKE_AUTO${auto_set} TRUE)
            endif()
        endforeach()

        # Enable folder support for IDEs. A future CMake version might enable this by default.
        # See CMake issue #21695.
        get_property(__qt_use_folders GLOBAL PROPERTY USE_FOLDERS)
        if(__qt_use_folders OR "${__qt_use_folders}" STREQUAL "")
            set_property(GLOBAL PROPERTY USE_FOLDERS ON)
            get_property(__qt_qt_targets_folder GLOBAL PROPERTY QT_TARGETS_FOLDER)
            if("${__qt_qt_targets_folder}" STREQUAL "")
                set(__qt_qt_targets_folder QtInternalTargets)
                set_property(GLOBAL PROPERTY QT_TARGETS_FOLDER ${__qt_qt_targets_folder})
            endif()
            get_property(__qt_autogen_targets_folder GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDERS)
            if("${__qt_autogen_targets_folder}" STREQUAL "")
                set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER ${__qt_qt_targets_folder})
            endif()
        endif()
    endif()
endmacro()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    macro(qt_standard_project_setup)
        qt6_standard_project_setup(${ARGV})
    endmacro()
    macro(qt_policy)
        qt6_policy(${ARGV})
    endmacro()
endif()

function(qt6_generate_deploy_script)
    set(no_value_options "")
    set(single_value_options
        CONTENT
        OUTPUT_SCRIPT
        NAME
        TARGET

        # TODO: For backward compatibility / transitional use only,
        # remove at some point
        FILENAME_VARIABLE
    )
    set(multi_value_options "")
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unexpected arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()

    # TODO: Remove when FILENAME_VARIABLE is fully removed
    # Handle the slow deprecation of FILENAME_VARIABLE
    if(arg_FILENAME_VARIABLE)
        if(arg_OUTPUT_SCRIPT AND NOT arg_FILENAME_VARIABLE STREQUAL arg_OUTPUT_SCRIPT)
            message(FATAL_ERROR
                "Both FILENAME_VARIABLE and OUTPUT_SCRIPT were given and were different. "
                "Only one of the two should be used."
            )
        endif()
        message(AUTHOR_WARNING
            "The FILENAME_VARIABLE keyword is deprecated and will be removed soon. Please use OUTPUT_SCRIPT instead.")
        set(arg_OUTPUT_SCRIPT "${arg_FILENAME_VARIABLE}")
        unset(arg_FILENAME_VARIABLE)
    endif()

    if(NOT arg_OUTPUT_SCRIPT)
        message(FATAL_ERROR "OUTPUT_SCRIPT must be specified")
    endif()

    if("${arg_CONTENT}" STREQUAL "")
        message(FATAL_ERROR "CONTENT must be specified")
    endif()

    # Check whether manual finalization is needed.
    if(CMAKE_VERSION VERSION_LESS "3.19")
        get_target_property(is_immediately_finalized ${arg_TARGET} _qt_is_immediately_finalized)
        if(is_immediately_finalized)
            message(WARNING
                "Deployment of plugins for target '${arg_TARGET}' will not work. "
                "Either, upgrade CMake to version 3.19 or newer, or call "
                "qt_finalize_target(${arg_TARGET}) after generating the deployment script."
            )
        endif()
    endif()

    # Mark the target as "to be deployed".
    set_property(TARGET ${arg_TARGET} PROPERTY _qt_marked_for_deployment ON)

    # If the target already was finalized, maybe because it was defined in a subdirectory, generate
    # the plugin deployment information here.
    get_target_property(is_finalized "${arg_TARGET}" _qt_is_finalized)
    if(is_finalized)
        __qt_internal_generate_plugin_deployment_info(${arg_TARGET})
    endif()

    # Create a file name that will be unique for this target and the combination
    # of arguments passed to this command. This allows the project to call us
    # multiple times with different arguments for the same target (e.g. to
    # create deployment scripts for different scenarios).
    set(file_base_name "custom")
    if(NOT "${arg_NAME}" STREQUAL "")
        set(file_base_name "${arg_NAME}")
    elseif(NOT "${arg_TARGET}" STREQUAL "")
        set(file_base_name "${arg_TARGET}")
    endif()
    string(MAKE_C_IDENTIFIER "${file_base_name}" target_id)
    string(SHA1 args_hash "${ARGV}")
    string(SUBSTRING "${args_hash}" 0 10 short_hash)
    _qt_internal_get_deploy_impl_dir(deploy_impl_dir)
    set(deploy_script "${deploy_impl_dir}/deploy_${target_id}_${short_hash}")
    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        set(config_infix "-$<CONFIG>")
    else()
        set(config_infix "")
    endif()
    string(APPEND deploy_script "${config_infix}.cmake")
    set(${arg_OUTPUT_SCRIPT} "${deploy_script}" PARENT_SCOPE)

    set(boiler_plate "include(${QT_DEPLOY_SUPPORT})
include(\"\${CMAKE_CURRENT_LIST_DIR}/${arg_TARGET}-plugins${config_infix}.cmake\" OPTIONAL)
set(__QT_DEPLOY_ALL_MODULES_FOUND_VIA_FIND_PACKAGE \"${QT_ALL_MODULES_FOUND_VIA_FIND_PACKAGE}\")
")
    list(TRANSFORM arg_CONTENT REPLACE "\\$" "\$")
    file(GENERATE OUTPUT ${deploy_script} CONTENT "${boiler_plate}${arg_CONTENT}")
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    macro(qt_generate_deploy_script)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_generate_deploy_script(${ARGV})
        else()
            message(FATAL_ERROR "qt_generate_deploy_script() is only available in Qt 6.")
        endif()
    endmacro()
endif()

function(qt6_generate_deploy_app_script)
    # We use a TARGET keyword option instead of taking the target as the first
    # positional argument. This is to keep open the possibility of deploying
    # an app for which we don't have a target (e.g. an application from a
    # third party project that the caller may want to include in their own
    # package). We would add an EXECUTABLE keyword for that, which would be
    # mutually exclusive with the TARGET keyword.
    set(no_value_options
        NO_TRANSLATIONS
        NO_COMPILER_RUNTIME
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    set(single_value_options
        TARGET
        OUTPUT_SCRIPT

        # TODO: For backward compatibility / transitional use only,
        # remove at some point
        FILENAME_VARIABLE
    )
    set(qt_deploy_runtime_dependencies_options
        # These options are forwarded as is to qt_deploy_runtime_dependencies.
        PRE_INCLUDE_REGEXES
        PRE_EXCLUDE_REGEXES
        POST_INCLUDE_REGEXES
        POST_EXCLUDE_REGEXES
        POST_INCLUDE_FILES
        POST_EXCLUDE_FILES
    )
    set(multi_value_options
        ${qt_deploy_runtime_dependencies_options}
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unexpected arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT arg_TARGET)
        message(FATAL_ERROR "TARGET must be specified")
    endif()

    # TODO: Remove when FILENAME_VARIABLE is fully removed
    # Handle the slow deprecation of FILENAME_VARIABLE
    if(arg_FILENAME_VARIABLE)
        if(arg_OUTPUT_SCRIPT AND NOT arg_FILENAME_VARIABLE STREQUAL arg_OUTPUT_SCRIPT)
            message(FATAL_ERROR
                "Both FILENAME_VARIABLE and OUTPUT_SCRIPT were given and were different. "
                "Only one of the two should be used."
            )
        endif()
        message(AUTHOR_WARNING
            "The FILENAME_VARIABLE keyword is deprecated and will be removed soon. Please use OUTPUT_SCRIPT instead.")
        set(arg_OUTPUT_SCRIPT "${arg_FILENAME_VARIABLE}")
        unset(arg_FILENAME_VARIABLE)
    endif()

    if(NOT arg_OUTPUT_SCRIPT)
        message(FATAL_ERROR "OUTPUT_SCRIPT must be specified")
    endif()

    if(QT6_IS_SHARED_LIBS_BUILD)
        set(qt_build_type_string "shared Qt libs")
    else()
        set(qt_build_type_string "static Qt libs")
    endif()

    set(generate_args
        TARGET ${arg_TARGET}
        OUTPUT_SCRIPT deploy_script
    )

    set(common_deploy_args "")
    if(arg_NO_TRANSLATIONS)
        string(APPEND common_deploy_args "    NO_TRANSLATIONS\n")
    endif()
    if(arg_NO_COMPILER_RUNTIME)
        string(APPEND common_deploy_args "    NO_COMPILER_RUNTIME\n")
    endif()

    # Forward the arguments that are exactly the same for qt_deploy_runtime_dependencies.
    foreach(var IN LISTS qt_deploy_runtime_dependencies_options)
        if(NOT "${arg_${var}}" STREQUAL "")
            list(APPEND common_deploy_args ${var} ${arg_${var}})
        endif()
    endforeach()

    if(APPLE AND NOT IOS AND QT6_IS_SHARED_LIBS_BUILD)
        # TODO: Handle non-bundle applications if possible.
        get_target_property(is_bundle ${arg_TARGET} MACOSX_BUNDLE)
        if(NOT is_bundle)
            message(FATAL_ERROR
                "Executable targets have to be app bundles to use this command "
                "on Apple platforms."
            )
        endif()
        qt6_generate_deploy_script(${generate_args}
            CONTENT "
qt6_deploy_runtime_dependencies(
    EXECUTABLE $<TARGET_FILE_NAME:${arg_TARGET}>.app
${common_deploy_args})
")

    elseif(WIN32 AND QT6_IS_SHARED_LIBS_BUILD)
        qt6_generate_deploy_script(${generate_args}
            CONTENT "
qt6_deploy_runtime_dependencies(
    EXECUTABLE $<TARGET_FILE:${arg_TARGET}>
    GENERATE_QT_CONF
${common_deploy_args})
")

    elseif(UNIX AND NOT APPLE AND NOT ANDROID AND QT6_IS_SHARED_LIBS_BUILD)
        qt6_generate_deploy_script(${generate_args}
            CONTENT "
qt6_deploy_runtime_dependencies(
    EXECUTABLE $<TARGET_FILE:${arg_TARGET}>
    GENERATE_QT_CONF
${common_deploy_args})
")

    elseif(NOT arg_NO_UNSUPPORTED_PLATFORM_ERROR AND NOT QT_INTERNAL_NO_UNSUPPORTED_PLATFORM_ERROR)
        # Currently we don't deploy runtime dependencies if cross-compiling or using a static Qt.
        # We also don't do it if targeting Linux, but we could provide an option to do
        # so if we had a deploy tool or purely CMake-based deploy implementation.
        # Error out by default unless the project opted out of the error.
        # This provides us a migration path in the future without breaking compatibility promises.
        message(FATAL_ERROR
            "Support for installing runtime dependencies is not implemented for "
            "this target platform (${CMAKE_SYSTEM_NAME}, ${qt_build_type_string})."
        )
    else()
        qt6_generate_deploy_script(${generate_args}
            CONTENT "
_qt_internal_show_skip_runtime_deploy_message(\"${qt_build_type_string}\")
")
    endif()

    set(${arg_OUTPUT_SCRIPT} "${deploy_script}" PARENT_SCOPE)
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    macro(qt_generate_deploy_app_script)
        qt6_generate_deploy_app_script(${ARGV})
    endmacro()
endif()
