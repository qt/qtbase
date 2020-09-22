if(NOT QT_NO_INTERNAL_COMPATIBILITY_FUNCTIONS)
    # Compatibility functions that should be removed once all their usages are removed.
    function(extend_target)
        qt_extend_target(${ARGV})
    endfunction()

    function(add_qt_module)
        qt_add_module(${ARGV})
    endfunction()

    function(add_qt_plugin)
        qt_add_plugin(${ARGV})
    endfunction()

    function(add_qt_tool)
        qt_add_tool(${ARGV})
    endfunction()

    function(add_qt_test)
        qt_add_test(${ARGV})
    endfunction()

    function(add_qt_test_helper)
        qt_add_test_helper(${ARGV})
    endfunction()

    function(add_qt_manual_test)
        qt_add_manual_test(${ARGV})
    endfunction()

    function(add_qt_benchmark)
        qt_add_benchmark(${ARGV})
    endfunction()

    function(add_qt_executable)
        qt_add_executable(${ARGV})
    endfunction()

    function(add_qt_simd_part)
        qt_add_simd_part(${ARGV})
    endfunction()

    function(add_qt_docs)
        qt_add_docs(${ARGV})
    endfunction()

    function(add_qt_resource)
        qt_add_resource(${ARGV})
    endfunction()

    function(add_cmake_library)
        qt_add_cmake_library(${ARGV})
    endfunction()


    # New compatibility functions that should be removed before release.
    function(qt_extend_target)
        qt_internal_extend_target(${ARGV})
    endfunction()

    function(qt_add_module)
        qt_internal_add_module(${ARGV})
    endfunction()

    function(qt_add_tool)
        qt_internal_add_tool(${ARGV})
    endfunction()

    function(qt_add_test)
        qt_internal_add_test(${ARGV})
    endfunction()

    function(qt_add_test_helper)
        qt_internal_add_test_helper(${ARGV})
    endfunction()

    function(qt_add_manual_test)
        qt_internal_add_manual_test(${ARGV})
    endfunction()

    function(qt_add_benchmark)
        qt_internal_add_benchmark(${ARGV})
    endfunction()

    function(qt_add_executable)
        qt_internal_add_executable(${ARGV})
    endfunction()

    function(qt_add_simd_part)
        qt_internal_add_simd_part(${ARGV})
    endfunction()

    function(qt_add_docs)
        qt_internal_add_docs(${ARGV})
    endfunction()

    function(qt_add_resource)
        qt_internal_add_resource(${ARGV})
    endfunction()

    function(qt_add_cmake_library)
        qt_internal_add_cmake_library(${ARGV})
    endfunction()

    function(qt_add_3rdparty_library)
        qt_internal_add_3rdparty_library(${ARGV})
    endfunction()

    function(qt_create_tracepoints)
        qt_internal_create_tracepoints(${ARGV})
    endfunction()
endif()
