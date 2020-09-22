# Handle files that need special SIMD-related flags.
# This creates an object library and makes target link
# to it (privately).
function(qt_internal_add_simd_part target)
    qt_parse_all_arguments(arg "qt_add_simd_part" "" ""
       "NAME;SIMD;${__default_private_args};COMPILE_FLAGS" ${ARGN})
    if ("x${arg_SIMD}" STREQUAL x)
        message(FATAL_ERROR "qt_add_simd_part needs a SIMD type to be set.")
    endif()

    set(condition "QT_FEATURE_${arg_SIMD}")
    string(TOUPPER "QT_CFLAGS_${arg_SIMD}" simd_flags_var_name)
    set(simd_flags_expanded "")

    # As per mkspecs/features/simd.prf, the arch_haswell SIMD compiler is enabled when
    # qmake's CONFIG contains "avx2", which maps to CMake's QT_FEATURE_avx2.
    # The list of dependencies 'avx2 bmi bmi2 f16c fma lzcnt popcnt' only influences whether
    # the 'arch_haswell' SIMD flags need to be added explicitly to the compiler invocation.
    # If the compiler adds them implicitly, they must be present in qmake's QT_CPU_FEATURES as
    # detected by the architecture test, and thus they are present in TEST_subarch_result.
    if("${arg_SIMD}" STREQUAL arch_haswell)
        set(condition "QT_FEATURE_avx2")

        # Use avx2 flags as per simd.prf, if there are no specific arch_haswell flags specified in
        # QtCompilerOptimization.cmake.
        if("${simd_flags_var_name}" STREQUAL "")
            set(simd_flags_var_name "QT_CFLAGS_AVX2")
        endif()

    # The avx512 profiles dependencies DO influence if the SIMD compiler will be executed,
    # so each of the profile dependencies have to be in qmake's CONFIG for the compiler to be
    # enabled, which means the CMake features have to evaluate to true.
    # Also the profile flags to be used are a combination of arch_haswell, avx512f and each of the
    # dependencies.
    elseif("${arg_SIMD}" STREQUAL avx512common)
        set(condition "QT_FEATURE_avx512cd")
        list(APPEND simd_flags_expanded "${QT_CFLAGS_ARCH_HASWELL}")
        list(APPEND simd_flags_expanded "${QT_CFLAGS_AVX512F}")
        list(APPEND simd_flags_expanded "${QT_CFLAGS_AVX512CD}")
        list(REMOVE_DUPLICATES simd_flags_expanded)
    elseif("${arg_SIMD}" STREQUAL avx512core)
        set(condition "QT_FEATURE_avx512cd AND QT_FEATURE_avx512bw AND QT_FEATURE_avx512dq AND QT_FEATURE_avx512vl")
        list(APPEND simd_flags_expanded "${QT_CFLAGS_ARCH_HASWELL}")
        list(APPEND simd_flags_expanded "${QT_CFLAGS_AVX512F}")
        list(APPEND simd_flags_expanded "${QT_CFLAGS_AVX512CD}")
        list(APPEND simd_flags_expanded "${QT_CFLAGS_AVX512BW}")
        list(APPEND simd_flags_expanded "${QT_CFLAGS_AVX512DQ}")
        list(APPEND simd_flags_expanded "${QT_CFLAGS_AVX512VL}")
        list(REMOVE_DUPLICATES simd_flags_expanded)
    endif()

    set(name "${arg_NAME}")
    if("x${name}" STREQUAL x)
        set(name "${target}_simd_${arg_SIMD}")
    endif()

    qt_evaluate_config_expression(result ${condition})
    if(${result})
        if(QT_CMAKE_DEBUG_EXTEND_TARGET)
            message("qt_add_simd_part(${target} SIMD ${arg_SIMD} ...): Evaluated")
        endif()

        if(NOT simd_flags_expanded)
            set(simd_flags_expanded "${${simd_flags_var_name}}")
        endif()

        foreach(source IN LISTS arg_SOURCES)
            set_property(SOURCE "${source}" APPEND
                PROPERTY COMPILE_OPTIONS
                ${simd_flags_expanded}
                ${arg_COMPILE_FLAGS}
            )
        endforeach()
        set_source_files_properties(${arg_SOURCES} PROPERTIES SKIP_PRECOMPILE_HEADERS TRUE)
        target_sources(${target} PRIVATE ${arg_SOURCES})
    else()
        if(QT_CMAKE_DEBUG_EXTEND_TARGET)
            message("qt_add_simd_part(${target} SIMD ${arg_SIMD} ...): Skipped")
        endif()
    endif()
endfunction()
