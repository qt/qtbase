# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


# WARNING must keep in sync with wasm-emscripten/qmake.conf!
function (qt_internal_setup_wasm_target_properties wasmTarget)

    target_link_options("${wasmTarget}" INTERFACE
    "SHELL:-s MAX_WEBGL_VERSION=2"
    "SHELL:-s FETCH=1"
    "SHELL:-s WASM_BIGINT=1")

    target_link_libraries("${wasmTarget}" INTERFACE embind)

    ## wasm64
    if (WASM64)
        target_compile_options("${wasmTarget}" INTERFACE "SHELL:-s MEMORY64=1" )
        target_link_options("${wasmTarget}" INTERFACE   "SHELL:-s MEMORY64=1" -mwasm64)
    endif()
    # Enable MODULARIZE and set EXPORT_NAME, which makes it possible to
    # create application instances using a global constructor function,
    # e.g. let app_instance = await createQtAppInstance().
    # (as opposed to MODULARIZE=0, where Emscripten creates a global app
    # instance object at Javascript eval time)
    target_link_options("${wasmTarget}" INTERFACE
    "SHELL:-s MODULARIZE=1"
    "SHELL:-s EXPORT_NAME=createQtAppInstance")

    #simd
    if (QT_FEATURE_wasm_simd128)
        target_compile_options("${wasmTarget}" INTERFACE -msimd128)
    endif()
    if (QT_FEATURE_sse2)
        target_compile_options("${wasmTarget}" INTERFACE -O2 -msimd128 -msse -msse2)
    endif()

    # wasm exceptions
    if (QT_FEATURE_wasm_exceptions)
        target_compile_options("${wasmTarget}" INTERFACE -fwasm-exceptions)
        target_link_options("${wasmTarget}" INTERFACE -fwasm-exceptions)
    else()
        target_link_options("${wasmTarget}" INTERFACE "SHELL:-s DISABLE_EXCEPTION_CATCHING=1")
    endif()

    if (QT_FEATURE_thread)
        target_compile_options("${wasmTarget}" INTERFACE "SHELL:-pthread")
        target_link_options("${wasmTarget}" INTERFACE "SHELL:-pthread")
    endif()

    target_link_options("${wasmTarget}" INTERFACE "SHELL:-s ALLOW_MEMORY_GROWTH")

    # debug add_compile_options
    if ("QT_WASM_SOURCE_MAP=1" IN_LIST QT_QMAKE_DEVICE_OPTIONS)
        set(WASM_SOURCE_MAP_BASE "http://localhost:8000/")

        if(DEFINED QT_WASM_SOURCE_MAP_BASE)
            set(WASM_SOURCE_MAP_BASE "${QT_WASM_SOURCE_MAP_BASE}")
        endif()

        # Pass --source-map-base on the linker line. This informs the
        # browser where to find the source files when debugging.
        # -g4 to make source maps for debugging
        target_link_options("${wasmTarget}" INTERFACE  "-gsource-map" "--source-map-base" "${WASM_SOURCE_MAP_BASE}")

    endif()

    # a few good defaults to make console more verbose while debugging
    target_link_options("${wasmTarget}" INTERFACE $<$<CONFIG:Debug>:
        "SHELL:-s DEMANGLE_SUPPORT=1"
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

    #  Set ASYNCIFY_IMPORTS unconditionally in order to support enabling asyncify at link time.
    target_link_options("${wasmTarget}" INTERFACE "SHELL:-sASYNCIFY_IMPORTS=qt_asyncify_suspend_js,qt_asyncify_resume_js")

    if(QT_FEATURE_shared)

        set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)

        # plugins are SIDE_MODULE
        target_compile_options("${wasmTarget}" INTERFACE
        "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:" -s SIDE_MODULE=1>)
        target_link_options("${wasmTarget}" INTERFACE
        "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:" -s SIDE_MODULE=1>)

        # shared libs are SIDE_MODULE
        target_compile_options("${wasmTarget}" INTERFACE
        "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:" -s SIDE_MODULE=1>)
        target_link_options("${wasmTarget}" INTERFACE
        "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:" -s SIDE_MODULE=1>)

    else()
        target_link_options("${wasmTarget}" INTERFACE "SHELL:-s ERROR_ON_UNDEFINED_SYMBOLS=1")
    endif()

    # Suppress warnings for known issues for developer builds
    if(FEATURE_developer_build)
        target_link_options("${wasmTarget}" INTERFACE "SHELL:-Wno-pthreads-mem-growth")
    endif()

endfunction()

function(qt_internal_wasm_add_finalizers target)
    qt_add_list_file_finalizer(_qt_internal_add_wasm_extra_exported_methods ${target})
    qt_add_list_file_finalizer(_qt_internal_wasm_add_target_helpers ${target})
endfunction()


