# Copy in Qt HTML/JS launch files for apps.
function(_qt_internal_wasm_add_target_helpers target)
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

        # Hardcode wasm memory size.
        get_target_property(_tmp_initialMemory "${target}" QT_WASM_INITIAL_MEMORY)
        if(_tmp_initialMemory)
            set(QT_WASM_INITIAL_MEMORY "${_tmp_initialMemory}")
        elseif(NOT DEFINED QT_WASM_INITIAL_MEMORY)
            if(QT_FEATURE_thread)
                # Pthreads and ALLOW_MEMORY_GROWTH can cause javascript wasm memory access to
                # be slow and having to update HEAP* views. Instead, we specify the memory size
                # at build time. Further, browsers limit the maximum initial memory size to 1GB.
                # https://github.com/WebAssembly/design/issues/1271
                set(QT_WASM_INITIAL_MEMORY "1GB")
            else()
                # emscripten default is 16MB, we need slightly more sometimes
                set(QT_WASM_INITIAL_MEMORY "50MB")
            endif()
        endif()

        if(DEFINED QT_WASM_INITIAL_MEMORY)
            # QT_WASM_INITIAL_MEMORY must be a multiple of 65536
            target_link_options("${target}"
                PRIVATE "SHELL:-s INITIAL_MEMORY=${QT_WASM_INITIAL_MEMORY}")
            message(DEBUG "-- Setting INITIAL_MEMORY to ${QT_WASM_INITIAL_MEMORY} for ${target}")
        endif()

    endif()
endfunction()

