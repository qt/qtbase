# Copy in Qt HTML/JS launch files for apps.
function(_qt_internal_wasm_add_target_helpers target)

    _qt_test_emscripten_version()
    get_target_property(targetType "${target}" TYPE)
    if("${targetType}" STREQUAL "EXECUTABLE")

        if(QT6_INSTALL_PREFIX)
            set(WASM_BUILD_DIR "${QT6_INSTALL_PREFIX}")
        elseif(QT_BUILD_DIR)
            set(WASM_BUILD_DIR "${QT_BUILD_DIR}")
        endif()

        get_target_property(output_name ${target} OUTPUT_NAME)
        if(output_name)
            set(_target_output_name "${output_name}")
        else()
            set(_target_output_name "${target}")
        endif()

        set(APPNAME ${_target_output_name})

        get_target_property(target_output_directory ${target} RUNTIME_OUTPUT_DIRECTORY)

        if(target_output_directory)
            set(_target_directory "${target_output_directory}")
        else()
            set(_target_directory "${CMAKE_CURRENT_BINARY_DIR}")
        endif()

        configure_file("${WASM_BUILD_DIR}/plugins/platforms/wasm_shell.html"
            "${_target_directory}/${_target_output_name}.html")
        configure_file("${WASM_BUILD_DIR}/plugins/platforms/qtloader.js"
            ${_target_directory}/qtloader.js COPYONLY)
        configure_file("${WASM_BUILD_DIR}/plugins/platforms/qtlogo.svg"
            ${_target_directory}/qtlogo.svg COPYONLY)

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

function(_qt_internal_add_wasm_extra_exported_methods target)
    get_target_property(wasm_extra_exported_methods "${target}" QT_WASM_EXTRA_EXPORTED_METHODS)

    if(NOT wasm_extra_exported_methods)
        set(wasm_extra_exported_methods ${QT_WASM_EXTRA_EXPORTED_METHODS})
    endif()

    if(wasm_extra_exported_methods)
        target_link_options("${target}" PRIVATE
        "SHELL:-s EXPORTED_RUNTIME_METHODS=UTF16ToString,stringToUTF16,${wasm_extra_exported_methods}"
        )
    else()
        # an errant dangling comma will break this
        target_link_options("${target}" PRIVATE
            "SHELL:-s EXPORTED_RUNTIME_METHODS=UTF16ToString,stringToUTF16"
        )
    endif()
endfunction()
