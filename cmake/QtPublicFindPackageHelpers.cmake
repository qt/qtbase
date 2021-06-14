function(qt_internal_disable_find_package_global_promotion target)
    set_target_properties("${target}" PROPERTIES _qt_no_promote_global TRUE)
endfunction()
