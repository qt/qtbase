# Computes which sanitizer options should be set based on features evaluated in qtbase.
# Sets ECM_ENABLE_SANITIZERS with those options in the function calling scope.
function(qt_internal_set_up_sanitizer_options)
    set(ECM_ENABLE_SANITIZERS "" CACHE STRING "Enable sanitizers")
    set_property(CACHE ECM_ENABLE_SANITIZERS PROPERTY STRINGS
        "address;memory;thread;undefined;fuzzer;fuzzer-no-link")

    # If QT_FEATURE_sanitize_foo was enabled, make sure to set the appropriate
    # ECM_ENABLE_SANITIZERS value.
    set(enabled_sanitizer_features "")
    foreach(sanitizer_type address memory thread undefined)
        if(QT_FEATURE_sanitize_${sanitizer_type})
            list(APPEND enabled_sanitizer_features "${sanitizer_type}")
        endif()
    endforeach()

    # There's a mismatch between fuzzer-no-link ECM option and fuzzer_no_link Qt feature.
    if(QT_FEATURE_sanitize_fuzzer_no_link)
        list(APPEND enabled_sanitizer_features "fuzzer-no-link")
    endif()

    if(enabled_sanitizer_features)
        set(ECM_ENABLE_SANITIZERS "${enabled_sanitizer_features}" PARENT_SCOPE)
    endif()
endfunction()
