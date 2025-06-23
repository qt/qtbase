# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_moc Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_moc LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

set(JSON_HEADERS
        backslash-newlines.h
        c-comments.h
        cstyle-enums.h
        cxx11-enums.h
        cxx11-explicit-override-control.h
        cxx11-final-classes.h
        cxx11-trailing-return.h
        cxx17-namespaces.h
        dir-in-include-path.h
        escapes-in-string-literals.h
        enum_with_include.h
        forward-declared-param.h
        function-with-attributes.h
        gadgetwithnoenums.h
        grand-parent-gadget-class.h
        moc_include.h
        namespace.h
        namespaced-flags.h
        namespaced-base-class.h
        no-keywords.h
        non-gadget-parent-class.h
        oldstyle-casts.h
        parse-defines.h
        plugin_metadata.h
        pointery_to_incomplete.h
        pure-virtual-signals.h
        qflags64object.h
        qinvokable.h
        qmlmacro.h
        qprivateslots.h
        qtbug-35657-gadget.h
        related-metaobjects-in-gadget.h
        related-metaobjects-in-namespaces.h
        related-metaobjects-name-conflict.h
        signal-with-default-arg.h
        single-quote-digit-separator-n3781.h
        slots-with-void-template.h
        task192552.h
        task234909.h
        task240368.h
        task87883.h
        tech-preview.h
        trigraphs.h
        using-namespaces.h
)

qt_wrap_cpp(comparison_relevant_moc_list ${JSON_HEADERS}
    TARGET tst_moc
    OPTIONS
        "-Muri=com.company.app"
        "-Muri=com.company.app.private"
        "-DDEFINE_CMDLINE_EMPTY="
        "-DDEFINE_CMDLINE_SIGNAL=void cmdlineSignal(const QMap<int, int> &i)"
        "--output-json"
)

list(TRANSFORM comparison_relevant_moc_list  APPEND ".json" OUTPUT_VARIABLE moc_json_files)

qt_internal_add_test(tst_moc
    SOURCES
        cxx-attributes.h
        single_function_keyword.h
        name_collision.h
        tst_moc.cpp
        ${comparison_relevant_moc_list}
    INCLUDE_DIRECTORIES
        testproject
        testproject/include
    MOC_OPTIONS
        "-Muri=com.company.app"
        "-Muri=com.company.app.private"
        "-DDEFINE_CMDLINE_EMPTY="
        "-DDEFINE_CMDLINE_SIGNAL=void cmdlineSignal(const QMap<int, int> &i)"
        "--output-json"
    EXCEPTIONS
)

qt_internal_extend_target(tst_moc LIBRARIES Qt::CorePrivate)

qt_internal_extend_target(tst_moc CONDITION CMAKE_CROSSCOMPILING
    DEFINES
        MOC_CROSS_COMPILED
)

if (${PROJECT_VERSION_MAJOR} LESS 7)
    # This file intentional produces a warning, so we use qt_wrap_cpp so we can
    # suppress it.
    qt_wrap_cpp(flags_property_moc flags-property-integer-access.h)
    qt_internal_extend_target(tst_moc SOURCES ${flags_property_moc})
    if (CLANG OR GCC)
        set(Wno_deprecated "-Wno-deprecated-declarations")
    elseif (MSVC)
        # warning isn't triggering
        set(Wno_deprecated "-wd4996")
    endif()
    set_source_files_properties(${flags_property_moc} COMPILE_FLAGS "${Wno_deprecated}")
endif()

if (UNIX AND (CLANG OR GCC OR QCC))
    qt_wrap_cpp(os9_moc os9-newlines.h)
endif()

qt_internal_extend_target(tst_moc CONDITION UNIX AND (CLANG OR GCC OR QCC)
    SOURCES
        os9-newlines.h
        win-newlines.h
        ${os9_moc}
)

qt_internal_extend_target(tst_moc CONDITION CLANG OR GCC
    SOURCES
        dollars.h
)

qt_internal_extend_target(tst_moc CONDITION TARGET Qt::DBus
    LIBRARIES
        Qt::DBus
)

qt_internal_extend_target(tst_moc CONDITION TARGET Qt::Concurrent
    LIBRARIES
        Qt::Concurrent
)

qt_internal_extend_target(tst_moc CONDITION TARGET Qt::Network
    LIBRARIES
        Qt::Network
)

qt_internal_extend_target(tst_moc CONDITION TARGET Qt::Sql
    LIBRARIES
        Qt::Sql
)

get_target_property(target_binary_dir tst_moc BINARY_DIR)
set(cmake_autogen_cache_file
    "${target_binary_dir}/CMakeFiles/tst_moc_autogen.dir/ParseCache.txt")
set(cmake_autogen_info_file
    "${target_binary_dir}/CMakeFiles/tst_moc_autogen.dir/AutogenInfo.json")
set(moc_json_out ${target_binary_dir}/moc_json_out)

file(REMOVE ${moc_json_out})
foreach(filename ${moc_json_files})
    file(APPEND ${moc_json_out} "${filename}\n")
endforeach()

set(metatype_file "allmocs.json")
configure_file("allmocs_baseline_in.json" "${target_binary_dir}/allmocs_baseline.json")

add_custom_command(TARGET tst_moc
    POST_BUILD
    COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::moc
        -o "allmocs.json"
        --collect-json  "@${moc_json_out}"
    COMMENT "Running moc with --collect-json"
    VERBATIM
)

# Add dependencies that are implicitly used inside the test
add_dependencies(tst_moc
    Qt::qtpaths
    Qt::moc
)
