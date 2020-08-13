# Compatibility macros that should be removed once all their usages are removed.
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
