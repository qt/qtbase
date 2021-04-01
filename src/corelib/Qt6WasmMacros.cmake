
function(qt6_wasm_add_target_helpers target)
    # copy in Qt HTML/JS launch files for apps
    get_target_property(targetType "${target}" TYPE)
    if("${targetType}" STREQUAL "EXECUTABLE")

        set(APPNAME ${target})

        if(QT6_INSTALL_PREFIX)
            set(WASM_BUILD_DIR "${QT6_INSTALL_PREFIX}")
        elseif(QT_BUILD_DIR)
            set(WASM_BUILD_DIR "${QT_BUILD_DIR}")
        endif()

        configure_file("${WASM_BUILD_DIR}/plugins/platforms/wasm_shell.html"
            "${target}.html")
        configure_file("${WASM_BUILD_DIR}/plugins/platforms/qtloader.js"
            qtloader.js COPYONLY)
        configure_file("${WASM_BUILD_DIR}/plugins/platforms/qtlogo.svg"
            qtlogo.svg COPYONLY)

    endif()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_wasm_add_target_helpers)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_wasm_add_target_helpers(${ARGV})
        endif()
    endfunction()
endif()
