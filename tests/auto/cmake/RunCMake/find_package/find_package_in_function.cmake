# Finding Qt inside a function hides Qt's variables from the deferred finalizer.
function(add_library_in_function target)
    find_package(Qt6 REQUIRED COMPONENTS Core)
    qt_add_library(${target})
    target_sources(${target} PRIVATE dummy.cpp)
    target_link_libraries(${target} PRIVATE Qt6::Core)
endfunction()

add_library_in_function(foo)
