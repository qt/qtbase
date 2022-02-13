
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

        if(QT_FEATURE_thread)
            set(POOL_SIZE 4)
            get_target_property(_tmp_poolSize "${target}" QT_WASM_PTHREAD_POOL_SIZE)
            if(_tmp_poolSize)
                set(POOL_SIZE ${_tmp_poolSize})
            elseif(DEFINED QT_WASM_PTHREAD_POOL_SIZE)
                set(POOL_SIZE ${QT_WASM_PTHREAD_POOL_SIZE})
            endif()
            target_link_options("${target}" PRIVATE "SHELL:-s PTHREAD_POOL_SIZE=${POOL_SIZE}")
            message(DEBUG "Setting PTHREAD_POOL_SIZE to ${POOL_SIZE} for ${target}")
        endif()

        # Hardcode wasm memory size. Emscripten does not currently support memory growth
        # (ALLOW_MEMORY_GROWTH) in pthreads mode, and requires specifying the memory size
        # at build time. Further, browsers limit the maximum initial memory size to 1GB.
        # QT_WASM_INITIAL_MEMORY must be a multiple of 64KB (i.e. 65536)
        get_target_property(_tmp_initialMemory "${target}" QT_WASM_INITIAL_MEMORY)
        if(_tmp_initialMemory)
            set(QT_WASM_INITIAL_MEMORY "${_tmp_initialMemory}")
        elseif(NOT DEFINED QT_WASM_INITIAL_MEMORY)
            if(QT_FEATURE_thread)
                set(QT_WASM_INITIAL_MEMORY "1GB")
            else()
                # emscripten default is 16MB, we need slightly more sometimes
                set(QT_WASM_INITIAL_MEMORY "20MB")
            endif()
        endif()

        if(DEFINED QT_WASM_INITIAL_MEMORY)
            target_link_options("${target}"
                PRIVATE "SHELL:-s INITIAL_MEMORY=${QT_WASM_INITIAL_MEMORY}")
            message(DEBUG "-- Setting INITIAL_MEMORY to ${QT_WASM_INITIAL_MEMORY} for ${target}")
        endif()

    endif()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_wasm_add_target_helpers)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_wasm_add_target_helpers(${ARGV})
        endif()
    endfunction()
endif()
