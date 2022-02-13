
function (qt_internal_setup_wasm_target_properties wasmTarget)

    target_link_options("${wasmTarget}" INTERFACE "SHELL:-s EXIT_RUNTIME=1"
    "SHELL:-s ERROR_ON_UNDEFINED_SYMBOLS=1"
    "SHELL:-s EXTRA_EXPORTED_RUNTIME_METHODS=[UTF16ToString,stringToUTF16]"
    "SHELL:-s USE_WEBGL2=1"
    "--bind"
    "SHELL:-s FETCH=1")
    target_compile_options("${wasmTarget}" INTERFACE --bind)

    #simd
    if (QT_FEATURE_sse2)
        target_compile_options("${wasmTarget}" INTERFACE -O2 -msimd128 -msse -msse2)
    endif()

    if (QT_FEATURE_opengles3)
        target_link_options("${wasmTarget}" INTERFACE "SHELL:-s FULL_ES3=1")

        target_link_options("${wasmTarget}" INTERFACE "SHELL:-s FULL_ES3=1"
            "SHELL:-s MAX_WEBGL_VERSION=2"
            "SHELL:-s WEBGL2_BACKWARDS_COMPATIBILITY_EMULATION=1")
    else()
        target_link_options("${wasmTarget}" INTERFACE "SHELL:-s FULL_ES2=1")
    endif()

    set(disable_exceptions_catching 1)
    if (QT_FEATURE_exceptions)
        set(disable_exceptions_catching 0)
    endif()
    target_link_options("${wasmTarget}" INTERFACE "SHELL:-s DISABLE_EXCEPTION_CATCHING=${disable_exceptions_catching}")

    if (QT_FEATURE_thread)
        target_compile_options("${wasmTarget}" INTERFACE "SHELL:-s USE_PTHREADS=1")
        target_link_options("${wasmTarget}" INTERFACE "SHELL:-s USE_PTHREADS=1")
    else()
        target_link_options("${wasmTarget}" INTERFACE "SHELL:-s ALLOW_MEMORY_GROWTH=1")
    endif()

    # debug add_compile_options
    if ("QT_WASM_SOURCE_MAP=1" IN_LIST QT_QMAKE_DEVICE_OPTIONS)
        set(WASM_SOURCE_MAP_BASE "http://localhost:8000/")

        if(DEFINED QT_WASM_SOURCE_MAP_BASE)
            set(WASM_SOURCE_MAP_BASE "${QT_WASM_SOURCE_MAP_BASE}")
        endif()

        # Pass --source-map-base on the linker line. This informs the
        # browser where to find the source files when debugging.
        # -g4 to make source maps for debugging
        target_link_options("${wasmTarget}" INTERFACE  "-g4" "--source-map-base" "${WASM_SOURCE_MAP_BASE}")

    endif()

    # a few good defaults to make console more verbose while debugging
    target_link_options("${wasmTarget}" INTERFACE $<$<CONFIG:Debug>:
        "SHELL:-s DEMANGLE_SUPPORT=1"
        "SHELL:-s GL_DEBUG=1"
        "SHELL:-s ASSERTIONS=2"
        --profiling-funcs>)

    # target_link_options("${wasmTarget}" INTERFACE "SHELL:-s LIBRARY_DEBUG=1") # print out library calls, verbose
    # target_link_options("${wasmTarget}" INTERFACE "SHELL:-s SYSCALL_DEBUG=1") # print out sys calls, verbose
    # target_link_options("${wasmTarget}" INTERFACE "SHELL:-s FS_LOG=1") # print out filesystem ops, verbose
    # target_link_options("${wasmTarget}" INTERFACE "SHELL:-s SOCKET_DEBUG") # print out socket,network data transfer

    if ("QT_EMSCRIPTEN_ASYNCIFY=1" IN_LIST QT_QMAKE_DEVICE_OPTIONS)

        # Emscripten recommends building with optimizations when using asyncify
        # in order to reduce wasm file size, and may also generate broken wasm
        # (with "wasm validation error: too many locals" type errors) if optimizations
        # are omitted. Enable optimizations also for debug builds.
        set(QT_CFLAGS_OPTIMIZE_DEBUG "-Os" CACHE STRING INTERNAL FORCE)
        set(QT_FEATURE_optimize_debug ON CACHE BOOL INTERNAL FORCE)

        target_link_options("${wasmTarget}" INTERFACE "SHELL:-s ASYNCIFY" "-Os")
        target_compile_definitions("${wasmTarget}" INTERFACE QT_HAVE_EMSCRIPTEN_ASYNCIFY)
    endif()
endfunction()
