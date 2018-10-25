function(qt_feature_module_begin)
    qt_parse_all_arguments(_arg "qt_feature_module_begin"
        "" "LIBRARY;PRIVATE_FILE;PUBLIC_FILE" "PUBLIC_DEPENDENCIES;PRIVATE_DEPENDENCIES" ${ARGN})

    if ("${_arg_LIBRARY}" STREQUAL "")
        message(FATAL_ERROR "qt_feature_begin_module needs a LIBRARY name!")
    endif()
    if ("${_arg_PUBLIC_FILE}" STREQUAL "")
        message(FATAL_ERROR "qt_feature_begin_module needs a PUBLIC_FILE name!")
    endif()
    if ("${_arg_PRIVATE_FILE}" STREQUAL "")
        message(FATAL_ERROR "qt_feature_begin_module needs a PRIVATE_FILE name!")
    endif()

    set(__QtFeature_library "${_arg_LIBRARY}" PARENT_SCOPE)
    set(__QtFeature_private_features "" PARENT_SCOPE)
    set(__QtFeature_public_features "" PARENT_SCOPE)

    set(__QtFeature_private_file "${_arg_PRIVATE_FILE}" PARENT_SCOPE)
    set(__QtFeature_public_file "${_arg_PUBLIC_FILE}" PARENT_SCOPE)

    set(__QtFeature_private_extra "" PARENT_SCOPE)
    set(__QtFeature_public_extra "" PARENT_SCOPE)

    qt_push_features_into_parent_scope(PUBLIC_FEATURES ${_arg_PUBLIC_DEPENDENCIES})
    qt_push_features_into_parent_scope(PRIVATE_FEATURES ${_arg_PRIVATE_DEPENDENCIES})
endfunction()

function(qt_feature _feature)
    qt_parse_all_arguments(_arg "qt_feature"
        "PRIVATE;PUBLIC"
        "LABEL;PURPOSE;SECTION;" "AUTODETECT;CONDITION;ENABLE;DISABLE;EMIT_IF" ${ARGN})

    set(_QT_FEATURE_DEFINITION_${_feature} ${ARGN} PARENT_SCOPE)

    # Register feature for future use:
    if (_arg_PUBLIC)
        list(APPEND __QtFeature_public_features "${_feature}")
    endif()
    if (_arg_PRIVATE)
        list(APPEND __QtFeature_private_features "${_feature}")
    endif()

    set(__QtFeature_public_features ${__QtFeature_public_features} PARENT_SCOPE)
    set(__QtFeature_private_features ${__QtFeature_private_features} PARENT_SCOPE)
endfunction()

function(qt_evaluate_to_boolean expressionVar)
    if(${${expressionVar}})
        set(${expressionVar} ON PARENT_SCOPE)
    else()
        set(${expressionVar} OFF PARENT_SCOPE)
    endif()
endfunction()

function(qt_evaluate_config_expression resultVar)
    set(result "")
    set(nestingLevel 0)
    set(skipNext OFF)
    set(expression "${ARGN}")
    list(LENGTH expression length)

    math(EXPR length "${length}-1")
    foreach(memberIdx RANGE ${length})
        if(${skipNext})
            set(skipNext OFF)
            continue()
        endif()

        list(GET expression ${memberIdx} member)

        if("${member}" STREQUAL "(")
            if(${nestingLevel} GREATER 0)
                list(APPEND result ${member})
            endif()
            math(EXPR nestingLevel "${nestingLevel} + 1")
            continue()
        elseif("${member}" STREQUAL ")")
            math(EXPR nestingLevel "${nestingLevel} - 1")
            if(nestingLevel LESS 0)
                break()
            endif()
            if(${nestingLevel} EQUAL 0)
                qt_evaluate_config_expression(result ${result})
            else()
                list(APPEND result ${member})
            endif()
            continue()
        elseif(${nestingLevel} GREATER 0)
            list(APPEND result ${member})
            continue()
        elseif("${member}" STREQUAL "NOT")
            list(APPEND result ${member})
            continue()
        elseif("${member}" STREQUAL "AND")
            qt_evaluate_to_boolean(result)
            if(NOT ${result})
                break()
            endif()
            set(result "")
        elseif("${member}" STREQUAL "OR")
            qt_evaluate_to_boolean(result)
            if(${result})
                break()
            endif()
            set(result "")
        elseif("${member}" STREQUAL "STREQUAL" AND memberIdx LESS ${length})
            # Unfortunately the semantics for STREQUAL in if() are broken when the
            # RHS is an empty string and the parameters to if are coming through a variable.
            # So we expect people to write the empty string with single quotes and then we
            # do the comparison manually here.
            list(LENGTH result lhsIndex)
            math(EXPR lhsIndex "${lhsIndex}-1")
            list(GET result ${lhsIndex} lhs)
            list(REMOVE_AT result ${lhsIndex})
            set(lhs "${${lhs}}")

            math(EXPR rhsIndex "${memberIdx}+1")
            set(skipNext ON)

            list(GET expression ${rhsIndex} rhs)
            # We can't pass through an empty string with double quotes through various
            # stages of substitution, so instead it is represented using single quotes
            # and resolve here.
            string(REGEX REPLACE "'(.*)'" "\\1" rhs "${rhs}")

            string(COMPARE EQUAL "${lhs}" "${rhs}" stringCompareResult)
            list(APPEND result ${stringCompareResult})
        else()
            string(FIND "${member}" "QT_FEATURE_" idx)
            if(idx EQUAL 0)
                # Remove the QT_FEATURE_ prefix
                string(SUBSTRING "${member}" 11 -1 feature)
                qt_evaluate_feature(${feature})
            endif()

            list(APPEND result ${member})
        endif()
    endforeach()

    if("${result}" STREQUAL "")
        set(result ON)
    else()
        qt_evaluate_to_boolean(result)
    endif()

    set(${resultVar} ${result} PARENT_SCOPE)
endfunction()

function(qt_evaluate_feature _feature)
    # If the feature was set explicitly by the user to be on or off, in the cache, then
    # there's nothing for us to do.
    if(DEFINED "QT_FEATURE_${_feature}")
        return()
    endif()

    if(NOT DEFINED _QT_FEATURE_DEFINITION_${_feature})
        message(FATAL_ERROR "Attempting to evaluate feature ${_feature} but its definition is missing. Either the feature does not exist or a dependency to the module that defines it is missing")
    endif()

    cmake_parse_arguments(_arg
        "PRIVATE;PUBLIC"
        "LABEL;PURPOSE;SECTION;" "AUTODETECT;CONDITION;ENABLE;DISABLE;EMIT_IF" ${_QT_FEATURE_DEFINITION_${_feature}})

    if(DEFINED _QT_FEATURE_VALUE_${_feature})
        set(QT_FEATURE_${_feature} ${_QT_FEATURE_VALUE_${_feature}} PARENT_SCOPE)
        return()
    endif()

    if("${_arg_ENABLE}" STREQUAL "")
        set(_arg_ENABLE OFF)
    endif()

    if("${_arg_DISABLE}" STREQUAL "")
        set(_arg_DISABLE OFF)
    endif()

    if("${_arg_AUTODETECT}" STREQUAL "")
        set(_arg_AUTODETECT ON)
    endif()

    if("${_arg_CONDITION}" STREQUAL "")
        set(condition ON)
    else()
        qt_evaluate_config_expression(condition ${_arg_CONDITION})
    endif()

    if(${_arg_DISABLE})
        set(result OFF)
    elseif((${_arg_ENABLE}) OR (${_arg_AUTODETECT}))
        set(result ${condition})
    else()
        # feature not auto-detected and not explicitly enabled
        set(result OFF)
    endif()

    if("${_arg_EMIT_IF}" STREQUAL "")
        set(emit_if ON)
    else()
        qt_evaluate_config_expression(emit_if ${_arg_EMIT_IF})
    endif()

    if (${emit_if})
        set(FEATURE_${_feature} "UNSET" CACHE STRING "${_arg_LABEL}")
        set_property(CACHE FEATURE_${_feature} PROPERTY STRINGS UNSET ON OFF)
    endif()

    set(_QT_FEATURE_VALUE_${_feature} "${result}" CACHE INTERNAL "${_arg_LABEL}")
    set(QT_FEATURE_${_feature} "${result}" PARENT_SCOPE)
endfunction()

function(qt_feature_definition _feature _name)
    qt_parse_all_arguments(_arg "qt_feature_definition" "NEGATE" "VALUE" "" ${ARGN})

    # Generate code:
    set(_expected 1)
    if (_arg_NEGATE)
        set(_expected -1)
    endif()
    set(_msg "\n#if defined(QT_FEATURE_${_feature}) && QT_FEATURE_${_feature} == ${_expected}\n")
    if (_arg_VALUE)
        string(APPEND _msg "#  define ${_name} ${_arg_VALUE}\n")
    else()
        string(APPEND _msg "#  define ${_name}\n")
    endif()
    string(APPEND _msg "#endif\n")

    # Store for later use:
    list(FIND __QtFeature_public_features "${_feature}" _public_index)
    if (_public_index GREATER -1)
        string(APPEND __QtFeature_public_extra "${_msg}")
    endif()
    list(FIND __QtFeature_private_features "${_feature}" _private_index)
    if (_private_index GREATER -1)
        string(APPEND __QtFeature_private_extra "${_msg}")
    endif()

    set(__QtFeature_public_extra ${__QtFeature_public_extra} PARENT_SCOPE)
    set(__QtFeature_private_extra ${__QtFeature_private_extra} PARENT_SCOPE)
endfunction()

function(qt_extra_definition _name _value)
    qt_parse_all_arguments(_arg "qt_extra_definition" "PUBLIC;PRIVATE" "" "" ${ARGN})

    if (_arg_PUBLIC)
        string(APPEND __QtFeature_public_extra "\n#define ${_name} ${_value}\n")
    elseif(_arg_PRIVATE)
        string(APPEND __QtFeature_private_extra "\n#define ${_name} ${_value}\n")
    endif()

    set(__QtFeature_public_extra ${__QtFeature_public_extra} PARENT_SCOPE)
    set(__QtFeature_private_extra ${__QtFeature_private_extra} PARENT_SCOPE)
endfunction()

function(_qt_generate_feature_line _line _feature)
    set(_line "")
    if (QT_FEATURE_${_feature} STREQUAL "ON")
        set(_line "#define QT_FEATURE_${_feature} 1\n\n" PARENT_SCOPE)
    elseif(QT_FEATURE_${_feature} STREQUAL "OFF")
        set(_line "#define QT_FEATURE_${_feature} -1\n\n" PARENT_SCOPE)
    elseif(QT_FEATURE_${_feature} STREQUAL "UNSET")
        set(_line "#define QT_FEATURE_${_feature} 0\n\n" PARENT_SCOPE)
    else()
        message(FATAL_ERROR "${_feature} has unexpected value \"${QT_FEATURE_${_feature}}\"!")
    endif()
endfunction()

function(_qt_feature_write_file _file _features _extra)
    message("Generating file ${_file}.")
    set(_contents "")
    foreach(_it ${_features})
        _qt_generate_feature_line(_line "${_it}")
        string(APPEND _contents "${_line}")
    endforeach()
    string(APPEND _contents "${_extra}")

    file(GENERATE OUTPUT "${_file}" CONTENT "${_contents}")
endfunction()

function(qt_feature_module_end target)
    set(all_features ${__QtFeature_public_features} ${__QtFeature_private_features})
    list(REMOVE_DUPLICATES all_features)

    foreach(feature ${all_features})
        unset(_QT_FEATURE_VALUE_${feature} CACHE)
    endforeach()

    foreach(feature ${all_features})
        qt_evaluate_feature(${feature})

        set(QT_FEATURE_${feature} ${QT_FEATURE_${feature}} PARENT_SCOPE)
    endforeach()

    set(enabled_public_features "")
    set(disabled_public_features "")
    set(enabled_private_features "")
    set(disabled_private_features "")

    foreach(feature ${__QtFeature_public_features})
       if(QT_FEATURE_${feature})
          list(APPEND enabled_public_features ${feature})
       else()
          list(APPEND disabled_public_features ${feature})
       endif()
    endforeach()

    foreach(feature ${__QtFeature_private_features})
       if(QT_FEATURE_${feature})
          list(APPEND enabled_private_features ${feature})
       else()
          list(APPEND disabled_private_features ${feature})
       endif()
    endforeach()

    foreach(feature ${all_features})
        unset(_QT_FEATURE_VALUE_${feature} CACHE)
        unset(_QT_FEATURE_DEFINITION_${feature} CACHE)
    endforeach()

    _qt_feature_write_file("${CMAKE_CURRENT_BINARY_DIR}/${__QtFeature_private_file}"
        "${__QtFeature_private_features}" "${__QtFeature_private_extra}"
    )
    qt_generate_forwarding_headers("${__QtFeature_library}" SOURCE "${__QtFeature_private_file}" PRIVATE)

    _qt_feature_write_file("${CMAKE_CURRENT_BINARY_DIR}/${__QtFeature_public_file}"
        "${__QtFeature_public_features}" "${__QtFeature_public_extra}"
    )
    qt_generate_forwarding_headers("${__QtFeature_library}" SOURCE "${__QtFeature_public_file}")

    get_target_property(targetType "${target}" TYPE)
    if("${targetType}" STREQUAL "INTERFACE_LIBRARY")
        set(propertyPrefix "INTERFACE_")
    else()
        set(propertyPrefix "")
        set_target_properties("${target}" PROPERTIES EXPORT_PROPERTIES "QT_ENABLED_PUBLIC_FEATURES;QT_DISABLED_PUBLIC_FEATURES;QT_ENABLED_PRIVATE_FEATURES;QT_DISABLED_PRIVATE_FEATURES")
    endif()
    foreach(visibility public private)
        string(TOUPPER "${visibility}" capitalVisibility)
        foreach(state enabled disabled)
            string(TOUPPER "${state}" capitalState)

            set_property(TARGET "${target}" PROPERTY ${propertyPrefix}QT_${capitalState}_${capitalVisibility}_FEATURES "${${state}_${visibility}_features}")
        endforeach()
    endforeach()

    unset(__QtFeature_library PARENT_SCOPE)
    unset(__QtFeature_private_features PARENT_SCOPE)
    unset(__QtFeature_public_features PARENT_SCOPE)

    unset(__QtFeature_private_file PARENT_SCOPE)
    unset(__QtFeature_public_file PARENT_SCOPE)

    unset(__QtFeature_private_extra PARENT_SCOPE)
    unset(__QtFeature_public_extra PARENT_SCOPE)
endfunction()

function(qt_config_compile_test name)
    cmake_parse_arguments(_arg "" "LABEL" "" ${ARGN})

    check_cxx_source_compiles("${_arg_UNPARSED_ARGUMENTS}" HAVE_${name})
    set(TEST_${name} "${HAVE_${name}}" CACHE INTERNAL "${_arg_LABEL}")
endfunction()

function(qt_config_compile_test_x86simd extension label)
    string(TOUPPER ${extension} extension_uppercase)
    try_compile(TEST_X86SIMD_${extension} "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/config.tests/x86_simd/main.cpp"
        COMPILE_DEFINITIONS -DQT_COMPILER_SUPPORTS_${extension_uppercase}
        OUTPUT_VARIABLE foo
    )
    set(TEST_subarch_${extension} "${TEST_X86SIMD_${extension}}" CACHE INTERNAL "${label}" )
endfunction()

function(qt_pull_features_into_current_scope)
    cmake_parse_arguments(__arg "PUBLIC_FEATURES;PRIVATE_FEATURES" "FEATURE_PROPERTY_INFIX" "" ${ARGN})
    foreach(__target IN ITEMS ${__arg_UNPARSED_ARGUMENTS})
        if(NOT TARGET ${__target})
            continue()
        endif()
        get_target_property(__target_type "${__target}" TYPE)
        if("${__target_type}" STREQUAL "INTERFACE_LIBRARY")
            set(__property_prefix "INTERFACE_")
        else()
            set(__property_prefix "")
        endif()
        foreach(__visibility PUBLIC PRIVATE)
            set(__value ON)
            foreach(__state ENABLED DISABLED)
                if(NOT ${__arg_${__visibility}_FEATURES})
                    continue()
                endif()
                get_target_property(__features "${__target}" ${__property_prefix}QT_${__arg_FEATURE_PROPERTY_INFIX}${__state}_${__visibility}_FEATURES)
                if("${__features}" STREQUAL "__features-NOTFOUND")
                    continue()
                endif()
                foreach(__feature ${__features})
                    set(QT_FEATURE_${__feature} ${__value} PARENT_SCOPE)
                endforeach()
                set(__value OFF)
            endforeach()
        endforeach()
    endforeach()
endfunction()

macro(qt_push_features_into_parent_scope)
    cmake_parse_arguments(__arg "PUBLIC_FEATURES;PRIVATE_FEATURES" "FEATURE_PROPERTY_INFIX" "" ${ARGN})
    foreach(__target IN ITEMS ${__arg_UNPARSED_ARGUMENTS})
        if(NOT TARGET ${__target})
            continue()
        endif()
        get_target_property(__target_type "${__target}" TYPE)
        if("${__target_type}" STREQUAL "INTERFACE_LIBRARY")
            set(__property_prefix "INTERFACE_")
        else()
            set(__property_prefix "")
        endif()
        foreach(__visibility PUBLIC PRIVATE)
            set(__value ON)
            foreach(__state ENABLED DISABLED)
                if(NOT ${__arg_${__visibility}_FEATURES})
                    continue()
                endif()
                get_target_property(__features "${__target}" ${__property_prefix}QT_${__arg_FEATURE_PROPERTY_INFIX}${__state}_${__visibility}_FEATURES)
                if("${__features}" STREQUAL "__features-NOTFOUND")
                    continue()
                endif()
                foreach(__feature ${__features})
                    set(QT_FEATURE_${__feature} ${__value} PARENT_SCOPE)
                endforeach()
                set(__value OFF)
            endforeach()
        endforeach()
    endforeach()
endmacro()

macro(qt_load_global_features)
    if(NOT TARGET Qt::Core)
       find_package(Qt${PROJECT_VERSION_MAJOR}Core QUIET)
    endif()
    qt_pull_features_into_current_scope(PUBLIC_FEATURES PRIVATE_FEATURES FEATURE_PROPERTY_INFIX "GLOBAL_" Qt::Core)
endmacro()
