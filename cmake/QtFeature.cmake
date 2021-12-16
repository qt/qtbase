include(QtFeatureCommon)
include(CheckCXXCompilerFlag)

function(qt_feature_module_begin)
    qt_parse_all_arguments(arg "qt_feature_module_begin"
        "NO_MODULE;ONLY_EVALUATE_FEATURES"
        "LIBRARY;PRIVATE_FILE;PUBLIC_FILE" "PUBLIC_DEPENDENCIES;PRIVATE_DEPENDENCIES" ${ARGN})

    if(NOT arg_ONLY_EVALUATE_FEATURES)
        if ("${arg_LIBRARY}" STREQUAL "" AND (NOT ${arg_NO_MODULE}))
            message(FATAL_ERROR
                    "qt_feature_begin_module needs a LIBRARY name! (or specify NO_MODULE)")
        endif()
        if ("${arg_PUBLIC_FILE}" STREQUAL "")
            message(FATAL_ERROR "qt_feature_begin_module needs a PUBLIC_FILE name!")
        endif()
        if ("${arg_PRIVATE_FILE}" STREQUAL "")
            message(FATAL_ERROR "qt_feature_begin_module needs a PRIVATE_FILE name!")
        endif()
        set(__QtFeature_only_evaluate_features OFF PARENT_SCOPE)
    else()
        set(__QtFeature_only_evaluate_features ON PARENT_SCOPE)
    endif()

    set(__QtFeature_library "${arg_LIBRARY}" PARENT_SCOPE)
    set(__QtFeature_public_features "" PARENT_SCOPE)
    set(__QtFeature_private_features "" PARENT_SCOPE)
    set(__QtFeature_internal_features "" PARENT_SCOPE)

    set(__QtFeature_private_file "${arg_PRIVATE_FILE}" PARENT_SCOPE)
    set(__QtFeature_public_file "${arg_PUBLIC_FILE}" PARENT_SCOPE)

    set(__QtFeature_private_extra "" PARENT_SCOPE)
    set(__QtFeature_public_extra "" PARENT_SCOPE)

    set(__QtFeature_config_definitions "" PARENT_SCOPE)

    set(__QtFeature_define_definitions "" PARENT_SCOPE)
endfunction()

function(qt_feature feature)
    set(original_name "${feature}")
    qt_feature_normalize_name("${feature}" feature)
    set_property(GLOBAL PROPERTY QT_FEATURE_ORIGINAL_NAME_${feature} "${original_name}")

    qt_parse_all_arguments(arg "qt_feature"
        "PRIVATE;PUBLIC"
        "LABEL;PURPOSE;SECTION;" "AUTODETECT;CONDITION;ENABLE;DISABLE;EMIT_IF" ${ARGN})

    set(_QT_FEATURE_DEFINITION_${feature} ${ARGN} PARENT_SCOPE)

    # Register feature for future use:
    if (arg_PUBLIC)
        list(APPEND __QtFeature_public_features "${feature}")
    endif()
    if (arg_PRIVATE)
        list(APPEND __QtFeature_private_features "${feature}")
    endif()
    if (NOT arg_PUBLIC AND NOT arg_PRIVATE)
        list(APPEND __QtFeature_internal_features "${feature}")
    endif()


    set(__QtFeature_public_features ${__QtFeature_public_features} PARENT_SCOPE)
    set(__QtFeature_private_features ${__QtFeature_private_features} PARENT_SCOPE)
    set(__QtFeature_internal_features ${__QtFeature_internal_features} PARENT_SCOPE)
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
    # The 'TARGET Gui' case is handled by qt_evaluate_to_boolean, by passing those tokens verbatim
    # to if().

    if("${result}" STREQUAL "")
        set(result ON)
    else()
        qt_evaluate_to_boolean(result)
    endif()

    set(${resultVar} ${result} PARENT_SCOPE)
endfunction()

function(_qt_internal_dump_expression_values expression_dump expression)
    set(dump "")
    set(skipNext FALSE)
    set(isTargetExpression FALSE)

    set(keywords "EQUAL" "LESS" "LESS_EQUAL" "GREATER" "GREATER_EQUAL" "STREQUAL" "STRLESS"
        "STRLESS_EQUAL" "STRGREATER" "STRGREATER_EQUAL" "VERSION_EQUAL" "VERSION_LESS"
        "VERSION_LESS_EQUAL" "VERSION_GREATER" "VERSION_GREATER_EQUAL" "MATCHES"
        "EXISTS" "COMMAND" "DEFINED" "NOT" "AND" "OR" "TARGET" "EXISTS" "IN_LIST" "(" ")")

    list(LENGTH expression length)
    math(EXPR length "${length}-1")

    if(${length} LESS 0)
        return()
    endif()

    foreach(memberIdx RANGE ${length})
        if(${skipNext})
            set(skipNext FALSE)
            continue()
        endif()

        list(GET expression ${memberIdx} member)
        if(NOT "${member}" IN_LIST keywords)
            string(FIND "${member}" "QT_FEATURE_" idx)
            if(idx EQUAL 0)
                if(NOT DEFINED ${member})
                    list(APPEND dump "${member} not evaluated")
                else()
                    list(APPEND dump "${member} = \"${${member}}\"")
                endif()
            elseif(isTargetExpression)
                set(targetExpression "TARGET;${member}")
                if(${targetExpression})
                    list(APPEND dump "TARGET ${member} found")
                else()
                    list(APPEND dump "TARGET ${member} not found")
                endif()
            else()
                list(APPEND dump "${member} = \"${${member}}\"")
            endif()
            set(isTargetExpression FALSE)
            set(skipNext FALSE)
        elseif("${member}" STREQUAL "TARGET")
            set(isTargetExpression TRUE)
        elseif("${member}" STREQUAL "STREQUAL")
            set(skipNext TRUE)
        else()
            set(skipNext FALSE)
            set(isTargetExpression FALSE)
        endif()
    endforeach()
    string(JOIN "\n    " ${expression_dump} ${dump})
    set(${expression_dump} "${${expression_dump}}" PARENT_SCOPE)
endfunction()

# Stores the user provided value to FEATURE_${feature} if provided.
# If not provided, stores ${computed} instead.
# ${computed} is also stored when reconfiguring and the condition does not align with the user
# provided value.
#
function(qt_feature_check_and_save_user_provided_value resultVar feature condition computed label)
    if (DEFINED "FEATURE_${feature}")
        # Revisit new user provided value
        set(user_value "${FEATURE_${feature}}")
        set(result "${user_value}")

        # If the build is marked as dirty and the user_value doesn't meet the new condition,
        # reset it to the computed one.
        get_property(dirty_build GLOBAL PROPERTY _qt_dirty_build)
        if(NOT condition AND result AND dirty_build)
            set(result "${computed}")
            message(WARNING "Reset FEATURE_${feature} value to ${result}, because it doesn't \
meet its condition after reconfiguration.")
        endif()

        set(bool_values OFF NO FALSE N ON YES TRUE Y)
        if ((result IN_LIST bool_values) OR (result GREATER_EQUAL 0))
            # All good!
        else()
            message(FATAL_ERROR
                "Sanity check failed: FEATURE_${feature} has invalid value \"${result}\"!")
        endif()

        # Fix-up user-provided values
        set("FEATURE_${feature}" "${result}" CACHE BOOL "${label}" FORCE)
    else()
        # Initial setup:
        set(result "${computed}")
        set("FEATURE_${feature}" "${result}" CACHE BOOL "${label}")
    endif()

    set("${resultVar}" "${result}" PARENT_SCOPE)
endfunction()

# Saves the final user value to QT_FEATURE_${feature}, after checking that the condition is met.
macro(qt_feature_check_and_save_internal_value
        feature saved_user_value condition label conditionExpression)
    if(${saved_user_value})
        set(result ON)
    else()
        set(result OFF)
    endif()

    if ((NOT condition) AND result)
        _qt_internal_dump_expression_values(conditionDump "${conditionExpression}")
        string(JOIN " " conditionString ${conditionExpression})
        qt_configure_add_report_error("Feature \"${feature}\": Forcing to \"${result}\" breaks its \
condition:\n    ${conditionString}\nCondition values dump:\n    ${conditionDump}\n" RECORD_ON_FEATURE_EVALUATION)
    endif()

    if (DEFINED "QT_FEATURE_${feature}")
        message(FATAL_ERROR "Feature ${feature} is already defined when evaluating configure.cmake features for ${target}.")
    endif()
    set(QT_FEATURE_${feature} "${result}" CACHE INTERNAL "Qt feature: ${feature}")

    # Add feature to build feature collection
    list(APPEND QT_KNOWN_FEATURES "${feature}")
    set(QT_KNOWN_FEATURES "${QT_KNOWN_FEATURES}" CACHE INTERNAL "" FORCE)
endmacro()


# The build system stores 2 CMake cache variables for each feature, to allow detecting value changes
# during subsequent reconfigurations.
#
#
# `FEATURE_foo` stores the user provided feature value for the current configuration run.
# It can be set directly by the user, or derived from INPUT_foo (also set by the user).
#
# If a value is not provided on initial configuration, the value will be auto-computed based on the
# various conditions of the feature.
# TODO: Document the various conditions and how they relate to each other.
#
#
# `QT_FEATURE_foo` stores the value of the feature from the previous configuration run.
# Its value is updated once with the newest user provided value after some checks are performed.
#
# This variable also serves as the main source of truth throughout the build system code to check
# if the feature is enabled, e.g. if(QT_FEATURE_foo)
#
# It is not meant to be set by the user. It is only modified by the build system.
#
# Comparing the values of QT_FEATURE_foo and FEATURE_foo, the build system can detect whether
# the user changed the value for a feature and thus recompute any dependent features.
#
function(qt_evaluate_feature feature)
    # If the feature was already evaluated as dependency nothing to do here.
    if(DEFINED "QT_FEATURE_${feature}")
        return()
    endif()

    if(NOT DEFINED _QT_FEATURE_DEFINITION_${feature})
        qt_debug_print_variables(DEDUP MATCH "^QT_FEATURE")
        message(FATAL_ERROR "Attempting to evaluate feature ${feature} but its definition is missing. Either the feature does not exist or a dependency to the module that defines it is missing")
    endif()

    cmake_parse_arguments(arg
        "PRIVATE;PUBLIC"
        "LABEL;PURPOSE;SECTION;" "AUTODETECT;CONDITION;ENABLE;DISABLE;EMIT_IF" ${_QT_FEATURE_DEFINITION_${feature}})

    if("${arg_ENABLE}" STREQUAL "")
        set(arg_ENABLE OFF)
    endif()

    if("${arg_DISABLE}" STREQUAL "")
        set(arg_DISABLE OFF)
    endif()

    if("${arg_AUTODETECT}" STREQUAL "")
        set(arg_AUTODETECT ON)
    endif()

    if("${arg_CONDITION}" STREQUAL "")
        set(condition ON)
    else()
        qt_evaluate_config_expression(condition ${arg_CONDITION})
    endif()

    qt_evaluate_config_expression(disable_result ${arg_DISABLE})
    qt_evaluate_config_expression(enable_result ${arg_ENABLE})
    qt_evaluate_config_expression(auto_detect ${arg_AUTODETECT})
    if(${disable_result})
        set(computed OFF)
    elseif((${enable_result}) OR (${auto_detect}))
        set(computed ${condition})
    else()
        # feature not auto-detected and not explicitly enabled
        set(computed OFF)
    endif()

    if("${arg_EMIT_IF}" STREQUAL "")
        set(emit_if ON)
    else()
        qt_evaluate_config_expression(emit_if ${arg_EMIT_IF})
    endif()

    # If FEATURE_ is not defined trying to use INPUT_ variable to enable/disable feature.
    if ((NOT DEFINED "FEATURE_${feature}") AND (DEFINED "INPUT_${feature}")
        AND (NOT "${INPUT_${feature}}" STREQUAL "undefined")
        AND (NOT "${INPUT_${feature}}" STREQUAL ""))
        if(INPUT_${feature})
            set(FEATURE_${feature} ON)
        else()
            set(FEATURE_${feature} OFF)
        endif()
    endif()

    # Warn about a feature which is not emitted, but the user explicitly provided a value for it.
    if(NOT emit_if AND DEFINED FEATURE_${feature})
        set(msg "")
        string(APPEND msg
            "Feature ${feature} is insignificant in this configuration, "
            "ignoring related command line option(s).")
        qt_configure_add_report_entry(TYPE WARNING MESSAGE "${msg}")

        # Remove the cache entry so that the warning is not persisted and shown on every
        # reconfiguration.
        unset(FEATURE_${feature} CACHE)
    endif()

    # Only save the user provided value if the feature was emitted.
    if(emit_if)
        qt_feature_check_and_save_user_provided_value(
            saved_user_value "${feature}" "${condition}" "${computed}" "${arg_LABEL}")
    else()
        # Make sure the feature internal value is OFF if not emitted.
        set(saved_user_value OFF)
    endif()

    qt_feature_check_and_save_internal_value(
        "${feature}" "${saved_user_value}" "${condition}" "${arg_LABEL}" "${arg_CONDITION}")

    # Store each feature's label for summary info.
    set(QT_FEATURE_LABEL_${feature} "${arg_LABEL}" CACHE INTERNAL "")
endfunction()

function(qt_feature_config feature config_var_name)
    qt_feature_normalize_name("${feature}" feature)
    qt_parse_all_arguments(arg "qt_feature_config" "NEGATE" "NAME" "" ${ARGN})

    # Store all the config related info in a unique variable key.
    set(key_name "_QT_FEATURE_CONFIG_DEFINITION_${feature}_${config_var_name}")
    set(${key_name} "FEATURE;${feature};CONFIG_VAR_NAME;${config_var_name};${ARGN}" PARENT_SCOPE)

    # Store the key for later evaluation.
    list(APPEND __QtFeature_config_definitions "${key_name}")

    set(__QtFeature_config_definitions ${__QtFeature_config_definitions} PARENT_SCOPE)
endfunction()

function(qt_evaluate_qmake_config_values key)
    if(NOT DEFINED ${key})
        qt_debug_print_variables(DEDUP MATCH "^_QT_FEATURE_CONFIG_DEFINITION")
        message(FATAL_ERROR
                "Attempting to evaluate feature config ${key} but its definition is missing. ")
    endif()

    cmake_parse_arguments(arg
        "NEGATE"
        "FEATURE;NAME;CONFIG_VAR_NAME"
        "" ${${key}})

    # If no custom name is specified, then the config value is the same as the feature name.
    if(NOT arg_NAME)
        set(arg_NAME "${arg_FEATURE}")
    endif()

    set(expected "NOT")
    if (arg_NEGATE)
        set(expected "")
        if(arg_NAME MATCHES "^no_(.*)")
            set(arg_NAME "${CMAKE_MATCH_1}")
        else()
            string(PREPEND arg_NAME "no_")
        endif()
    endif()

    # The feature condition is false, there is no need to export any config values.
    if(${expected} ${QT_FEATURE_${arg_FEATURE}})
        return()
    endif()

    if(arg_CONFIG_VAR_NAME STREQUAL "QMAKE_PUBLIC_CONFIG")
        list(APPEND __QtFeature_qmake_public_config "${arg_NAME}")
        set(__QtFeature_qmake_public_config "${__QtFeature_qmake_public_config}" PARENT_SCOPE)
    endif()
    if(arg_CONFIG_VAR_NAME STREQUAL "QMAKE_PRIVATE_CONFIG")
        list(APPEND __QtFeature_qmake_private_config "${arg_NAME}")
        set(__QtFeature_qmake_private_config "${__QtFeature_qmake_private_config}" PARENT_SCOPE)
    endif()
    if(arg_CONFIG_VAR_NAME STREQUAL "QMAKE_PUBLIC_QT_CONFIG")
        list(APPEND __QtFeature_qmake_public_qt_config "${arg_NAME}")
        set(__QtFeature_qmake_public_qt_config "${__QtFeature_qmake_public_qt_config}" PARENT_SCOPE)
    endif()
endfunction()

function(qt_feature_definition feature name)
    qt_feature_normalize_name("${feature}" feature)
    qt_parse_all_arguments(arg "qt_feature_definition" "NEGATE" "VALUE;PREREQUISITE" "" ${ARGN})

    # Store all the define related info in a unique variable key.
    set(key_name "_QT_FEATURE_DEFINE_DEFINITION_${feature}_${name}")
    set(${key_name} "FEATURE;${feature};NAME;${name};${ARGN}" PARENT_SCOPE)

    # Store the key for later evaluation and subsequent define generation:
    list(APPEND __QtFeature_define_definitions "${key_name}")

    set(__QtFeature_define_definitions ${__QtFeature_define_definitions} PARENT_SCOPE)
endfunction()

function(qt_evaluate_feature_definition key)
    if(NOT DEFINED ${key})
        qt_debug_print_variables(DEDUP MATCH "^_QT_FEATURE_DEFINE_DEFINITION")
        message(FATAL_ERROR "Attempting to evaluate feature define ${key} but its definition is missing. ")
    endif()

    cmake_parse_arguments(arg
        "NEGATE;"
        "FEATURE;NAME;VALUE;PREREQUISITE" "" ${${key}})

    set(expected ON)
    if (arg_NEGATE)
        set(expected OFF)
    endif()

    set(actual OFF)
    if(QT_FEATURE_${arg_FEATURE})
        set(actual ON)
    endif()

    set(msg "")

    if(actual STREQUAL expected)
        set(indent "")
        if(arg_PREREQUISITE)
            string(APPEND msg "#if ${arg_PREREQUISITE}\n")
            set(indent "  ")
        endif()
        if (arg_VALUE)
            string(APPEND msg "${indent}#define ${arg_NAME} ${arg_VALUE}\n")
        else()
            string(APPEND msg "${indent}#define ${arg_NAME}\n")
        endif()
        if(arg_PREREQUISITE)
            string(APPEND msg "#endif\n")
        endif()

        string(APPEND __QtFeature_public_extra "${msg}")
    endif()

    set(__QtFeature_public_extra ${__QtFeature_public_extra} PARENT_SCOPE)
endfunction()

function(qt_extra_definition name value)
    qt_parse_all_arguments(arg "qt_extra_definition" "PUBLIC;PRIVATE" "" "" ${ARGN})

    if (arg_PUBLIC)
        string(APPEND __QtFeature_public_extra "\n#define ${name} ${value}\n")
    elseif(arg_PRIVATE)
        string(APPEND __QtFeature_private_extra "\n#define ${name} ${value}\n")
    endif()

    set(__QtFeature_public_extra ${__QtFeature_public_extra} PARENT_SCOPE)
    set(__QtFeature_private_extra ${__QtFeature_private_extra} PARENT_SCOPE)
endfunction()

function(qt_internal_generate_feature_line line feature)
    string(TOUPPER "${QT_FEATURE_${feature}}" value)
    if (value STREQUAL "ON")
        set(line "#define QT_FEATURE_${feature} 1\n\n" PARENT_SCOPE)
    elseif(value STREQUAL "OFF")
        set(line "#define QT_FEATURE_${feature} -1\n\n" PARENT_SCOPE)
    elseif(value STREQUAL "UNSET")
        set(line "#define QT_FEATURE_${feature} 0\n\n" PARENT_SCOPE)
    else()
        message(FATAL_ERROR "${feature} has unexpected value \"${QT_FEATURE_${feature}}\"! "
            "Valid values are ON, OFF and UNSET.")
    endif()
endfunction()

function(qt_internal_feature_write_file file features extra)
    set(contents "")
    foreach(it ${features})
        qt_internal_generate_feature_line(line "${it}")
        string(APPEND contents "${line}")
    endforeach()
    string(APPEND contents "${extra}")

    file(GENERATE OUTPUT "${file}" CONTENT "${contents}")
endfunction()

# Helper function which evaluates features from a given list of configure.cmake paths
# and creates the feature cache entries.
# Should not be used directly, unless features need to be available in a directory scope before the
# associated module evaluates the features.
# E.g. qtbase/src.pro needs access to Core features before src/corelib/CMakeLists.txt is parsed.
function(qt_feature_evaluate_features list_of_paths)
    qt_feature_module_begin(ONLY_EVALUATE_FEATURES)
    foreach(path ${list_of_paths})
        include("${path}")
    endforeach()
    qt_feature_module_end(ONLY_EVALUATE_FEATURES)
endfunction()

function(qt_feature_record_summary_entries list_of_paths)
    # Clean up any stale state just in case.
    qt_feature_unset_state_vars()

    set(__QtFeature_only_record_summary_entries TRUE)
    foreach(path ${list_of_paths})
        include("${path}")
    endforeach()
    qt_feature_unset_state_vars()
endfunction()

function(qt_feature_module_end)
    set(flags ONLY_EVALUATE_FEATURES)
    set(options OUT_VAR_PREFIX)
    set(multiopts)
    cmake_parse_arguments(arg "${flags}" "${options}" "${multiopts}" ${ARGN})
    set(target ${arg_UNPARSED_ARGUMENTS})

    # The value of OUT_VAR_PREFIX is used as a prefix for output variables that should be
    # set in the parent scope.
    if(NOT arg_OUT_VAR_PREFIX)
        set(arg_OUT_VAR_PREFIX "")
    endif()

    set(all_features ${__QtFeature_public_features} ${__QtFeature_private_features} ${__QtFeature_internal_features})
    list(REMOVE_DUPLICATES all_features)

    foreach(feature ${all_features})
        qt_evaluate_feature(${feature})
    endforeach()

    # Evaluate custom cache assignments.
    foreach(cache_var_name ${__QtFeature_custom_enabled_cache_variables})
        set(${cache_var_name} ON CACHE BOOL "Force enabled by platform requirements." FORCE)
    endforeach()
    foreach(cache_var_name ${__QtFeature_custom_disabled_cache_variables})
        set(${cache_var_name} OFF CACHE BOOL "Force disabled by platform requirements." FORCE)
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

    foreach(key ${__QtFeature_config_definitions})
        qt_evaluate_qmake_config_values(${key})
        unset(${key} PARENT_SCOPE)
    endforeach()

    foreach(key ${__QtFeature_define_definitions})
        qt_evaluate_feature_definition(${key})
        unset(${key} PARENT_SCOPE)
    endforeach()

    foreach(feature ${all_features})
        unset(_QT_FEATURE_DEFINITION_${feature} PARENT_SCOPE)
    endforeach()

    if(NOT arg_ONLY_EVALUATE_FEATURES)
        qt_internal_feature_write_file("${CMAKE_CURRENT_BINARY_DIR}/${__QtFeature_private_file}"
            "${__QtFeature_private_features}" "${__QtFeature_private_extra}"
        )

        qt_internal_feature_write_file("${CMAKE_CURRENT_BINARY_DIR}/${__QtFeature_public_file}"
            "${__QtFeature_public_features}" "${__QtFeature_public_extra}"
        )
    endif()

    # Extra header injections which have to have forwarding headers created by
    # qt_install_injections.
    # Skip creating forwarding headers if qt_feature_module_begin was called with NO_MODULE, aka
    # there is no include/<module_name> so there's no place to put the forwarding headers.
    if(__QtFeature_library)
        set(injections "")
        qt_compute_injection_forwarding_header("${__QtFeature_library}"
                                                 SOURCE "${__QtFeature_public_file}"
                                                 OUT_VAR injections)
        qt_compute_injection_forwarding_header("${__QtFeature_library}"
                                                SOURCE "${__QtFeature_private_file}" PRIVATE
                                                OUT_VAR injections)

        set(${arg_OUT_VAR_PREFIX}extra_library_injections ${injections} PARENT_SCOPE)
    endif()

    if (NOT ("${target}" STREQUAL "NO_MODULE") AND NOT arg_ONLY_EVALUATE_FEATURES)
        get_target_property(targetType "${target}" TYPE)
        if("${targetType}" STREQUAL "INTERFACE_LIBRARY")
            set(propertyPrefix "INTERFACE_")
        else()
            set(propertyPrefix "")
            set_property(TARGET "${target}" APPEND PROPERTY EXPORT_PROPERTIES "QT_ENABLED_PUBLIC_FEATURES;QT_DISABLED_PUBLIC_FEATURES;QT_ENABLED_PRIVATE_FEATURES;QT_DISABLED_PRIVATE_FEATURES;QT_QMAKE_PUBLIC_CONFIG;QT_QMAKE_PRIVATE_CONFIG;QT_QMAKE_PUBLIC_QT_CONFIG")
        endif()
        foreach(visibility public private)
            string(TOUPPER "${visibility}" capitalVisibility)
            foreach(state enabled disabled)
                string(TOUPPER "${state}" capitalState)

                set_property(TARGET "${target}" PROPERTY ${propertyPrefix}QT_${capitalState}_${capitalVisibility}_FEATURES "${${state}_${visibility}_features}")
            endforeach()
        endforeach()

        set_property(TARGET "${target}"
                    PROPERTY ${propertyPrefix}QT_QMAKE_PUBLIC_CONFIG
                    "${__QtFeature_qmake_public_config}")
        set_property(TARGET "${target}"
                     PROPERTY ${propertyPrefix}QT_QMAKE_PRIVATE_CONFIG
                     "${__QtFeature_qmake_private_config}")
        set_property(TARGET "${target}"
                     PROPERTY ${propertyPrefix}QT_QMAKE_PUBLIC_QT_CONFIG
                     "${__QtFeature_qmake_public_qt_config}")

        # Config values were the old-school features before actual configure.json features were
        # implemented. Therefore "CONFIG+=foo" values should be considered features as well,
        # so that CMake can find them when building qtmultimedia for example.
        if(__QtFeature_qmake_public_config)
            set_property(TARGET "${target}"
                         APPEND PROPERTY ${propertyPrefix}QT_ENABLED_PUBLIC_FEATURES
                         ${__QtFeature_qmake_public_config})
        endif()
        if(__QtFeature_qmake_private_config)
            set_property(TARGET "${target}"
                         APPEND PROPERTY ${propertyPrefix}QT_ENABLED_PRIVATE_FEATURES
                         ${__QtFeature_qmake_private_config})
        endif()
        if(__QtFeature_qmake_public_qt_config)
            set_property(TARGET "${target}"
                         APPEND PROPERTY ${propertyPrefix}QT_ENABLED_PUBLIC_FEATURES
                         ${__QtFeature_qmake_public_qt_config})
        endif()

        qt_feature_copy_global_config_features_to_core(${target})
    endif()

    qt_feature_unset_state_vars()
endfunction()

macro(qt_feature_unset_state_vars)
    unset(__QtFeature_library PARENT_SCOPE)
    unset(__QtFeature_public_features PARENT_SCOPE)
    unset(__QtFeature_private_features PARENT_SCOPE)
    unset(__QtFeature_internal_features PARENT_SCOPE)

    unset(__QtFeature_private_file PARENT_SCOPE)
    unset(__QtFeature_public_file PARENT_SCOPE)

    unset(__QtFeature_private_extra PARENT_SCOPE)
    unset(__QtFeature_public_extra PARENT_SCOPE)

    unset(__QtFeature_define_definitions PARENT_SCOPE)
    unset(__QtFeature_custom_enabled_features PARENT_SCOPE)
    unset(__QtFeature_custom_disabled_features PARENT_SCOPE)
    unset(__QtFeature_only_evaluate_features PARENT_SCOPE)
    unset(__QtFeature_only_record_summary_entries PARENT_SCOPE)
endmacro()

function(qt_feature_copy_global_config_features_to_core target)
    # CMake doesn't support setting custom properties on exported INTERFACE libraries
    # See https://gitlab.kitware.com/cmake/cmake/issues/19261.
    # To circumvent that, copy the properties from GlobalConfig to Core target.
    # This way the global features actually get set in the generated CoreTargets.cmake file.
    if(target STREQUAL Core)
        foreach(visibility public private)
            string(TOUPPER "${visibility}" capitalVisibility)
            foreach(state enabled disabled)
                string(TOUPPER "${state}" capitalState)

                set(core_property_name "QT_${capitalState}_${capitalVisibility}_FEATURES")
                set(global_property_name "INTERFACE_${core_property_name}")

                get_property(core_values TARGET Core PROPERTY ${core_property_name})
                get_property(global_values TARGET GlobalConfig PROPERTY ${global_property_name})

                set(total_values ${core_values} ${global_values})
                set_property(TARGET Core PROPERTY ${core_property_name} ${total_values})
            endforeach()
        endforeach()

        set(config_property_names
            QT_QMAKE_PUBLIC_CONFIG QT_QMAKE_PRIVATE_CONFIG QT_QMAKE_PUBLIC_QT_CONFIG )
        foreach(property_name ${config_property_names})
            set(core_property_name "${property_name}")
            set(global_property_name "INTERFACE_${core_property_name}")

            get_property(core_values TARGET Core PROPERTY ${core_property_name})
            get_property(global_values TARGET GlobalConfig PROPERTY ${global_property_name})

            set(total_values ${core_values} ${global_values})
            set_property(TARGET Core PROPERTY ${core_property_name} ${total_values})
        endforeach()
    endif()
endfunction()

function(qt_config_compile_test name)
    if(DEFINED "TEST_${name}")
        return()
    endif()

    cmake_parse_arguments(arg "" "LABEL;PROJECT_PATH;C_STANDARD;CXX_STANDARD"
        "COMPILE_OPTIONS;LIBRARIES;CODE;PACKAGES;CMAKE_FLAGS" ${ARGN})

    if(arg_PROJECT_PATH)
        message(STATUS "Performing Test ${arg_LABEL}")

        set(flags "")
        qt_get_platform_try_compile_vars(platform_try_compile_vars)
        list(APPEND flags ${platform_try_compile_vars})

        # If the repo has its own cmake modules, include those in the module path, so that various
        # find_package calls work.
        if(EXISTS "${PROJECT_SOURCE_DIR}/cmake")
            set(must_append_module_path_flag TRUE)
            set(flags_copy "${flags}")
            set(flags)
            foreach(flag IN LISTS flags_copy)
                if(flag MATCHES "^-DCMAKE_MODULE_PATH:STRING=")
                    set(must_append_module_path_flag FALSE)
                    set(flag "${flag}\\;${PROJECT_SOURCE_DIR}/cmake")
                endif()
                list(APPEND flags "${flag}")
            endforeach()
            if(must_append_module_path_flag)
                list(APPEND flags "-DCMAKE_MODULE_PATH:STRING=${PROJECT_SOURCE_DIR}/cmake")
            endif()
        endif()

        # Pass which packages need to be found.
        if(arg_PACKAGES)
            set(packages_list "")

            # Parse the package names, version, etc. An example would be:
            # PACKAGE Foo 6 REQUIRED
            # PACKAGE Bar 2 COMPONENTS Baz
            foreach(p ${arg_PACKAGES})
                if(p STREQUAL PACKAGE)
                    if(package_entry)
                        # Encode the ";" into "\;" to separate the arguments of a find_package call.
                        string(REPLACE ";" "\\;" package_entry_string "${package_entry}")
                        list(APPEND packages_list "${package_entry_string}")
                    endif()

                    set(package_entry "")
                else()
                    list(APPEND package_entry "${p}")
                endif()
            endforeach()
            # Parse final entry.
            if(package_entry)
                string(REPLACE ";" "\\;" package_entry_string "${package_entry}")
                list(APPEND packages_list "${package_entry_string}")
            endif()

            # Encode the ";" again.
            string(REPLACE ";" "\\;" packages_list "${packages_list}")

            # The flags are separated by ';', the find_package entries by '\;',
            # and the package parts of an entry by '\\;'.
            # Example:
            # WrapFoo\\;6\\;COMPONENTS\\;bar\;WrapBaz\\;5
            list(APPEND flags "-DQT_CONFIG_COMPILE_TEST_PACKAGES:STRING=${packages_list}")

            # Inside the project, the value of QT_CONFIG_COMPILE_TEST_PACKAGES is used in a foreach
            # loop that calls find_package() for each package entry, and thus the variable expansion
            # ends up calling something like find_package(WrapFoo;6;COMPONENTS;bar) aka
            # find_package(WrapFoo 6 COMPONENTS bar).
        endif()

        # Pass which libraries need to be linked against.
        if(arg_LIBRARIES)
            set(link_flags "")
            set(library_targets "")
            # Separate targets from link flags or paths. This is to prevent configuration failures
            # when the targets are not found due to missing packages.
            foreach(lib ${arg_LIBRARIES})
                string(FIND "${lib}" "::" is_library_target)
                if(is_library_target EQUAL -1)
                    list(APPEND link_flags "${lib}")
                else()
                    list(APPEND library_targets "${lib}")
                endif()
            endforeach()
            if(link_flags)
                list(APPEND flags "-DQT_CONFIG_COMPILE_TEST_LIBRARIES:STRING=${link_flags}")
            endif()
            if(library_targets)
                list(APPEND flags
                     "-DQT_CONFIG_COMPILE_TEST_LIBRARY_TARGETS:STRING=${library_targets}")
            endif()
        endif()

        # Pass override values for CMAKE_SYSTEM_{PREFIX|FRAMEWORK}_PATH.
        if(DEFINED QT_CMAKE_SYSTEM_PREFIX_PATH_BACKUP)
            set(path_list ${CMAKE_SYSTEM_PREFIX_PATH})
            string(REPLACE ";" "\\;" path_list "${path_list}")
            list(APPEND flags "-DQT_CONFIG_COMPILE_TEST_CMAKE_SYSTEM_PREFIX_PATH=${path_list}")
        endif()
        if(DEFINED QT_CMAKE_SYSTEM_FRAMEWORK_PATH_BACKUP)
            set(path_list ${CMAKE_SYSTEM_FRAMEWORK_PATH})
            string(REPLACE ";" "\\;" path_list "${path_list}")
            list(APPEND flags "-DQT_CONFIG_COMPILE_TEST_CMAKE_SYSTEM_FRAMEWORK_PATH=${path_list}")
        endif()

        if(NOT arg_CMAKE_FLAGS)
            set(arg_CMAKE_FLAGS "")
        endif()

        # CI passes the project dir of the Qt repository as absolute path without drive letter:
        #   \Users\qt\work\qt\qtbase
        # Ensure that arg_PROJECT_PATH is an absolute path with drive letter:
        #   C:/Users/qt/work/qt/qtbase
        # This works around CMake upstream issue #22534.
        if(CMAKE_HOST_WIN32)
            get_filename_component(arg_PROJECT_PATH "${arg_PROJECT_PATH}" REALPATH)
        endif()

        try_compile(HAVE_${name} "${CMAKE_BINARY_DIR}/config.tests/${name}" "${arg_PROJECT_PATH}"
                    "${name}" CMAKE_FLAGS ${flags} ${arg_CMAKE_FLAGS})

        if(${HAVE_${name}})
            set(status_label "Success")
        else()
            set(status_label "Failed")
        endif()
        message(STATUS "Performing Test ${arg_LABEL} - ${status_label}")
    else()
        foreach(library IN ITEMS ${arg_LIBRARIES})
            if(NOT TARGET "${library}")
                # If the dependency looks like a cmake target, then make this compile test
                # fail instead of cmake abort later via CMAKE_REQUIRED_LIBRARIES.
                string(FIND "${library}" "::" cmake_target_namespace_separator)
                if(NOT cmake_target_namespace_separator EQUAL -1)
                    set(HAVE_${name} FALSE)
                    break()
                endif()
            endif()
        endforeach()

        if(NOT DEFINED HAVE_${name})
            set(_save_CMAKE_C_STANDARD "${CMAKE_C_STANDARD}")
            set(_save_CMAKE_CXX_STANDARD "${CMAKE_CXX_STANDARD}")
            set(_save_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
            set(_save_CMAKE_TRY_COMPILE_PLATFORM_VARIABLES "${CMAKE_TRY_COMPILE_PLATFORM_VARIABLES}")

            if(arg_C_STANDARD)
               set(CMAKE_C_STANDARD "${arg_C_STANDARD}")
            endif()

            if(arg_CXX_STANDARD)
               set(CMAKE_CXX_STANDARD "${arg_CXX_STANDARD}")
            endif()

            set(CMAKE_REQUIRED_FLAGS ${arg_COMPILE_OPTIONS})

            # Pass -stdlib=libc++ on if necessary
            if (INPUT_stdlib_libcpp OR QT_FEATURE_stdlib_libcpp)
                list(APPEND CMAKE_REQUIRED_FLAGS "-stdlib=libc++")
            endif()

            # For MSVC we need to explicitly pass -Zc:__cplusplus to get correct __cplusplus
            # define values. According to common/msvc-version.conf the flag is supported starting
            # with 1913.
            # https://developercommunity.visualstudio.com/content/problem/139261/msvc-incorrectly-defines-cplusplus.html
            # No support for the flag in upstream CMake as of 3.17.
            # https://gitlab.kitware.com/cmake/cmake/issues/18837
            if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND MSVC_VERSION GREATER_EQUAL 1913)
                list(APPEND CMAKE_REQUIRED_FLAGS "-Zc:__cplusplus")
            endif()

            # Let CMake load our custom platform modules.
            if(NOT QT_AVOID_CUSTOM_PLATFORM_MODULES)
                list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES CMAKE_MODULE_PATH)
            endif()

            set(_save_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
            set(CMAKE_REQUIRED_LIBRARIES "${arg_LIBRARIES}")
            check_cxx_source_compiles("${arg_UNPARSED_ARGUMENTS} ${arg_CODE}" HAVE_${name})
            set(CMAKE_REQUIRED_LIBRARIES "${_save_CMAKE_REQUIRED_LIBRARIES}")

            set(CMAKE_C_STANDARD "${_save_CMAKE_C_STANDARD}")
            set(CMAKE_CXX_STANDARD "${_save_CMAKE_CXX_STANDARD}")
            set(CMAKE_REQUIRED_FLAGS "${_save_CMAKE_REQUIRED_FLAGS}")
            set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES "${_save_CMAKE_TRY_COMPILE_PLATFORM_VARIABLES}")
        endif()
    endif()

    set(TEST_${name} "${HAVE_${name}}" CACHE INTERNAL "${arg_LABEL}")
endfunction()

# This function should be used for passing required try compile platform variables to the
# project-based try_compile() call.
# out_var will be a list of -Dfoo=bar strings, suitable to pass to CMAKE_FLAGS.
function(qt_get_platform_try_compile_vars out_var)
    # Use the regular variables that are used for source-based try_compile() calls.
    set(flags "${CMAKE_TRY_COMPILE_PLATFORM_VARIABLES}")

    # Pass custom flags.
    list(APPEND flags "CMAKE_C_FLAGS")
    list(APPEND flags "CMAKE_C_FLAGS_DEBUG")
    list(APPEND flags "CMAKE_C_FLAGS_RELEASE")
    list(APPEND flags "CMAKE_C_FLAGS_RELWITHDEBINFO")
    list(APPEND flags "CMAKE_CXX_FLAGS")
    list(APPEND flags "CMAKE_CXX_FLAGS_DEBUG")
    list(APPEND flags "CMAKE_CXX_FLAGS_RELEASE")
    list(APPEND flags "CMAKE_CXX_FLAGS_RELWITHDEBINFO")
    list(APPEND flags "CMAKE_OBJCOPY")

    # Pass toolchain files.
    if(CMAKE_TOOLCHAIN_FILE)
        list(APPEND flags "CMAKE_TOOLCHAIN_FILE")
    endif()
    if(VCPKG_CHAINLOAD_TOOLCHAIN_FILE)
        list(APPEND flags "VCPKG_CHAINLOAD_TOOLCHAIN_FILE")
    endif()

    # Pass language standard flags.
    list(APPEND flags "CMAKE_C_STANDARD")
    list(APPEND flags "CMAKE_CXX_STANDARD")

    # Pass -stdlib=libc++ on if necessary
    if (INPUT_stdlib_libcpp OR QT_FEATURE_stdlib_libcpp)
        if(CMAKE_CXX_FLAGS)
            string(APPEND CMAKE_CXX_FLAGS " -stdlib=libc++")
        else()
            set(CMAKE_CXX_FLAGS "-stdlib=libc++")
        endif()
    endif()

    # Assemble the list with regular options.
    set(flags_cmd_line "")
    foreach(flag ${flags})
        if(${flag})
            list(APPEND flags_cmd_line "-D${flag}=${${flag}}")
        endif()
    endforeach()

    # Let CMake load our custom platform modules.
    if(NOT QT_AVOID_CUSTOM_PLATFORM_MODULES)
        list(APPEND flags_cmd_line "-DCMAKE_MODULE_PATH:STRING=${QT_CMAKE_DIR}/platforms")
    endif()

    # Pass darwin specific options.
    # The architectures need to be passed explicitly to project-based try_compile calls even on
    # macOS, so that arm64 compilation works on Apple silicon.
    qt_internal_get_first_osx_arch(osx_first_arch)
    if(osx_first_arch)
        # Do what qmake does, aka when doing a simulator_and_device build, build the
        # target architecture test only with the first given architecture, which should be the
        # device architecture, aka some variation of "arm" (armv7, arm64).
        list(APPEND flags_cmd_line "-DCMAKE_OSX_ARCHITECTURES:STRING=${osx_first_arch}")
    endif()
    if(UIKIT)
        # Specify the sysroot, but only if not doing a simulator_and_device build.
        # So keep the sysroot empty for simulator_and_device builds.
        if(QT_UIKIT_SDK)
            list(APPEND flags_cmd_line "-DCMAKE_OSX_SYSROOT:STRING=${QT_UIKIT_SDK}")
        endif()
    endif()
    if(QT_NO_USE_FIND_PACKAGE_SYSTEM_ENVIRONMENT_PATH)
        list(APPEND flags_cmd_line "-DCMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH:BOOL=OFF")
    endif()

    set("${out_var}" "${flags_cmd_line}" PARENT_SCOPE)
endfunction()

# Set out_var to the first value of CMAKE_OSX_ARCHITECTURES.
# Sets an empty string if no architecture is present.
function(qt_internal_get_first_osx_arch out_var)
    set(value "")
    if(CMAKE_OSX_ARCHITECTURES)
        list(GET CMAKE_OSX_ARCHITECTURES 0 value)
    endif()
    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

function(qt_config_compile_test_x86simd extension label)
    if (DEFINED TEST_X86SIMD_${extension})
        return()
    endif()

    set(flags "-DSIMD:string=${extension}")

    qt_get_platform_try_compile_vars(platform_try_compile_vars)
    list(APPEND flags ${platform_try_compile_vars})

    message(STATUS "Performing SIMD Test ${label}")
    try_compile("TEST_X86SIMD_${extension}"
        "${CMAKE_CURRENT_BINARY_DIR}/config.tests/x86_simd_${extension}"
        "${CMAKE_CURRENT_SOURCE_DIR}/config.tests/x86_simd"
        x86_simd
        CMAKE_FLAGS ${flags})
    if(${TEST_X86SIMD_${extension}})
        set(status_label "Success")
    else()
        set(status_label "Failed")
    endif()
    message(STATUS "Performing SIMD Test ${label} - ${status_label}")
    set(TEST_subarch_${extension} "${TEST_X86SIMD_${extension}}" CACHE INTERNAL "${label}")
endfunction()

function(qt_config_compile_test_machine_tuple label)
    if(DEFINED TEST_MACHINE_TUPLE OR NOT LINUX OR ANDROID)
        return()
    endif()

    message(STATUS "Performing Test ${label}")
    execute_process(COMMAND "${CMAKE_CXX_COMPILER}" -dumpmachine
        OUTPUT_VARIABLE output
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE exit_code)
    if(exit_code EQUAL 0)
        set(status_label "Success")
    else()
        set(status_label "Failed")
    endif()
    message(STATUS "Performing Test ${label} - ${status_label}")
    set(TEST_machine_tuple "${output}" CACHE INTERNAL "${label}")
endfunction()

function(qt_config_compiler_supports_flag_test name)
    if(DEFINED "TEST_${name}")
        return()
    endif()

    cmake_parse_arguments(arg "" "LABEL;FLAG" "" ${ARGN})
    check_cxx_compiler_flag("${arg_FLAG}" TEST_${name})
    set(TEST_${name} "${TEST_${name}}" CACHE INTERNAL "${label}")
endfunction()

# gcc expects -fuse-ld=mold (no absolute path can be given) (gcc >= 12.1)
#             or an 'ld' symlink to 'mold' in a dir that is passed via -B flag (gcc < 12.1)
#
# clang expects     -fuse-ld=mold
#                or -fuse-ld=<mold-abs-path>
#                or --ldpath=<mold-abs-path>  (clang >= 12)
# https://github.com/rui314/mold/#how-to-use
# TODO: In the gcc < 12.1 case, the qt_internal_check_if_linker_is_available(mold) check will
#       always return TRUE because gcc will not error out if it is given a -B flag pointing to an
#       invalid dir, as well as when the the symlink to the linker in the -B dir is not actually
#       a valid linker.
#       It would be nice to handle that case in a better way, but it's not that important
#       given that gcc > 12.1 now supports -fuse-ld=mold
# NOTE: In comparison to clang, in the gcc < 12.1 case, we pass the full path to where mold is
#       and that is recorded in PlatformCommonInternal's INTERFACE_LINK_OPTIONS target.
#       Moving such a Qt to a different machine and trying to build another repo won't
#       work because the recorded path will be invalid. This is not a problem with
#       the gcc >= 12.1 case
function(qt_internal_get_mold_linker_flags out_var)
    cmake_parse_arguments(PARSE_ARGV 1 arg "ERROR_IF_EMPTY" "" "")

    find_program(QT_INTERNAL_LINKER_MOLD mold)

    set(flag "")
    if(QT_INTERNAL_LINKER_MOLD)
        if(GCC)
            if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "12.1")
                set(flag "-fuse-ld=mold")
            else()
                set(mold_linker_dir "${CMAKE_CURRENT_BINARY_DIR}/.qt_linker")
                set(mold_linker_path "${mold_linker_dir}/ld")
                if(NOT EXISTS "${mold_linker_dir}")
                    file(MAKE_DIRECTORY "${mold_linker_dir}")
                endif()
                if(NOT EXISTS "${mold_linker_path}")
                    file(CREATE_LINK
                        "${QT_INTERNAL_LINKER_MOLD}"
                        "${mold_linker_path}"
                         SYMBOLIC)
                endif()
                set(flag "-B${mold_linker_dir}")
            endif()
        elseif(CLANG)
            if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "12")
                set(flag "--ld-path=mold")
            else()
                set(flag "-fuse-ld=mold")
            endif()
        endif()
    endif()
    if(arg_ERROR_IS_EMPTY AND NOT flag)
        message(FATAL_ERROR "Could not determine the flags to use the mold linker.")
    endif()
    set(${out_var} "${flag}" PARENT_SCOPE)
endfunction()

function(qt_internal_get_active_linker_flags out_var)
    set(flags "")
    if(GCC OR CLANG)
        if(QT_FEATURE_use_gold_linker)
            list(APPEND flags "-fuse-ld=gold")
        elseif(QT_FEATURE_use_bfd_linker)
            list(APPEND flags "-fuse-ld=bfd")
        elseif(QT_FEATURE_use_lld_linker)
            list(APPEND flags "-fuse-ld=lld")
        elseif(QT_FEATURE_use_mold_linker)
            qt_internal_get_mold_linker_flags(mold_flags ERROR_IF_EMPTY)
            list(APPEND flags "${mold_flags}")
        endif()
    endif()
    set(${out_var} "${flags}" PARENT_SCOPE)
endfunction()

function(qt_internal_check_if_linker_is_available name)
    if(DEFINED "TEST_${name}")
        return()
    endif()

    cmake_parse_arguments(arg "" "LABEL;FLAG" "" ${ARGN})
    set(flags "${arg_FLAG}")

    set(CMAKE_REQUIRED_LINK_OPTIONS ${flags})
    check_cxx_source_compiles("int main() { return 0; }" TEST_${name})
    set(TEST_${name} "${TEST_${name}}" CACHE INTERNAL "${label}")
endfunction()

function(qt_config_linker_supports_flag_test name)
    if(DEFINED "TEST_${name}")
        return()
    endif()

    cmake_parse_arguments(arg "" "LABEL;FLAG" "" ${ARGN})
    set(flags "-Wl,${arg_FLAG}")

    # Pass the linker that the main project uses to the compile test.
    qt_internal_get_active_linker_flags(linker_flags)
    if(linker_flags)
        list(PREPEND flags ${linker_flags})
    endif()

    set(CMAKE_REQUIRED_LINK_OPTIONS ${flags})
    check_cxx_source_compiles("int main() { return 0; }" TEST_${name})
    set(TEST_${name} "${TEST_${name}}" CACHE INTERNAL "${label}")
endfunction()

function(qt_make_features_available target)
    if(NOT "${target}" MATCHES "^${QT_CMAKE_EXPORT_NAMESPACE}::[a-zA-Z0-9_-]*$")
        message(FATAL_ERROR "${target} does not match ${QT_CMAKE_EXPORT_NAMESPACE}::[a-zA-Z0-9_-]*. INVALID NAME.")
    endif()
    if(NOT TARGET ${target})
        message(FATAL_ERROR "${target} not found.")
    endif()

    get_target_property(target_type "${target}" TYPE)
    if("${target_type}" STREQUAL "INTERFACE_LIBRARY")
        set(property_prefix "INTERFACE_")
    else()
        set(property_prefix "")
    endif()
    foreach(visibility IN ITEMS PUBLIC PRIVATE)
        set(value ON)
        foreach(state IN ITEMS ENABLED DISABLED)
            get_target_property(features "${target}" ${property_prefix}QT_${state}_${visibility}_FEATURES)
            if("${features}" STREQUAL "features-NOTFOUND")
                continue()
            endif()
            foreach(feature IN ITEMS ${features})
                if (DEFINED "QT_FEATURE_${feature}" AND NOT "${QT_FEATURE_${feature}}" STREQUAL "${value}")
                    message(FATAL_ERROR "Feature ${feature} is already defined to be \"${QT_FEATURE_${feature}}\" and should now be set to \"${value}\" when importing features from ${target}.")
                endif()
                set(QT_FEATURE_${feature} "${value}" CACHE INTERNAL "Qt feature: ${feature} (from target ${target})")
            endforeach()
            set(value OFF)
        endforeach()
    endforeach()
endfunction()
