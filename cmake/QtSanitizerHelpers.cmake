function(qt_internal_set_up_sanitizer_features)
    set(ECM_ENABLE_SANITIZERS "" CACHE STRING "Enable sanitizers")
    set_property(CACHE ECM_ENABLE_SANITIZERS PROPERTY STRINGS "address;memory;thread;undefined")

    # If FEATURE_sanitize_foo is set on the command line, make sure to set the appropriate
    # ECM_ENABLE_SANITIZERS value. Also the other way around. This basically allows setting either
    # the feature or ECM_ENABLE_SANITIZERS directly.
    #
    # TODO: Decide which one of these should be the source of truth, because reconfiguration with
    # different options might not work as expected when ECM_ENABLE_SANITIZERS is provided instead of
    # the features.
    set(enabled_sanitizer_features "")
    foreach(sanitizer_type address memory thread undefined)
        if(FEATURE_sanitize_${sanitizer_type})
            list(APPEND enabled_sanitizer_features "${sanitizer_type}")
        endif()
    endforeach()
    if(enabled_sanitizer_features)
        set(ECM_ENABLE_SANITIZERS
            "${enabled_sanitizer_features}" CACHE STRING "Enable sanitizers" FORCE)
    endif()

    if(ECM_ENABLE_SANITIZERS)
        foreach(sanitizer_type ${ECM_ENABLE_SANITIZERS})
            message(STATUS "Enabling sanitizer: ${sanitizer_type}")
            set(feature_name "FEATURE_sanitize_${sanitizer_type}")
            set(${feature_name} "ON" CACHE BOOL "Enable ${sanitizer_type} sanitizer" FORCE)
            set(QT_${feature_name} "ON" CACHE BOOL "Enable ${sanitizer_type} sanitizer" FORCE)
        endforeach()
    endif()
endfunction()
