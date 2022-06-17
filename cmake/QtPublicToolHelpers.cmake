# The function returns location of the imported 'tool', returns an empty string if tool is not
# imported.
function(__qt_internal_get_tool_imported_location out_var tool)
    unset(${out_var})
    if("${tool}" MATCHES "^Qt[0-9]?::.+$")
        # The tool target has namespace already
        set(target ${tool})
    else()
        set(target ${QT_CMAKE_EXPORT_NAMESPACE}::${tool})
    endif()

    if(NOT TARGET ${target})
        message(FATAL_ERROR "${target} is not a target.")
    endif()

    get_target_property(is_imported ${target} IMPORTED)
    if(NOT is_imported)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    get_target_property(configs ${target} IMPORTED_CONFIGURATIONS)
    list(TRANSFORM configs PREPEND _)
    # Well-known configuration types
    list(APPEND configs
        _RELWITHDEBINFO
        _RELEASE
        _MINSIZEREL
        _DEBUG
    )
    list(REMOVE_DUPLICATES configs)
    # Look for the default empty configuration type at the first place.
    list(PREPEND configs "")

    foreach(config ${configs})
        get_target_property(${out_var} ${target} "IMPORTED_LOCATION${config}")
        if(${out_var})
            break()
        endif()
    endforeach()

    set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()
