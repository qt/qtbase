## Test cases
# All test cases (libraries) are picked up and evaluated at the end of this file.
# Each test case is a dummy interface library.
# We don't really need the compiler options to be valid.
# Make sure to define a <test-case>_reference variable to compare with

add_library(basic INTERFACE)
qt_internal_add_compiler_dependent_flags(basic INTERFACE
    COMPILERS ALL
        OPTIONS -opt1
    COMPILERS GNU
        OPTIONS -opt2 -opt3
)
set(basic_reference
    "$<$<AND:$<COMPILE_LANGUAGE:CXX>>:-opt1>"
    "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU>>:-opt2;-opt3>"
)

add_library(conditions INTERFACE)
qt_internal_add_compiler_dependent_flags(conditions INTERFACE
    COMPILERS ALL
            # No conditions
            OPTIONS -opt
        # Multiple conditions
        CONDITIONS VERSION_LESS 42 AND $<BOOL:TRUE>
            OPTIONS -opt
        # A new condition loop
        CONDITIONS $<BOOL:FLASE>
            OPTIONS -opt
    COMPILERS GNU
            # New compiler set must reset to no conditions
            OPTIONS -opt
)
set(conditions_reference
    "$<$<AND:$<COMPILE_LANGUAGE:CXX>>:-opt>"
    "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<AND:$<VERSION_LESS:$<CXX_COMPILER_VERSION>,42>,$<BOOL:TRUE>>>:-opt>"
    "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<BOOL:FLASE>>:-opt>"
    "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU>>:-opt>"
)

add_library(conditions_stack INTERFACE)
qt_internal_add_compiler_dependent_flags(conditions_stack INTERFACE
    COMPILERS ALL
        # Conditions follow shortcircuit order
        CONDITIONS $<A> AND $<B> OR $<C>
            OPTIONS -opt
        # Check parenthesis
        CONDITIONS $<A> AND ($<B> OR $<C>)
            OPTIONS -opt
        # Consecutive AND are combined
        CONDITIONS $<A> AND $<B> AND $<C> OR $<D>
            OPTIONS -opt
)
set(conditions_stack_reference
    "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<OR:$<AND:$<A>,$<B>>,$<C>>>:-opt>"
    "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<AND:$<A>,$<OR:$<B>,$<C>>>>:-opt>"
    "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<OR:$<AND:$<A>,$<B>,$<C>>,$<D>>>:-opt>"
)

add_library(one_language INTERFACE)
qt_internal_add_compiler_dependent_flags(one_language INTERFACE
    COMPILERS ALL
        CONDITIONS VERSION_LESS 42
            OPTIONS -opt
    LANGUAGES C
)
set(one_language_reference
    "$<$<AND:$<COMPILE_LANGUAGE:C>,$<VERSION_LESS:$<C_COMPILER_VERSION>,42>>:-opt>"
)

add_library(multiple_language INTERFACE)
qt_internal_add_compiler_dependent_flags(multiple_language INTERFACE
    COMPILERS ALL
        CONDITIONS VERSION_LESS 42
            OPTIONS -opt
    LANGUAGES C OBJCXX
)
set(multiple_language_reference
    # Right now we always use CXX_COMPILER_VERSION even if not in LANGUAGES
    # Change this test case if we have a better interface
    "$<$<AND:$<COMPILE_LANGUAGE:C,OBJCXX>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,42>>:-opt>"
)

add_library(common_condition INTERFACE)
qt_internal_add_compiler_dependent_flags(common_condition INTERFACE
    COMPILERS ALL
            OPTIONS -opt
        CONDITIONS $<BOOL:other>
            OPTIONS -opt
    COMPILERS GNU
            OPTIONS -opt
    COMMON_CONDITIONS
        $<BOOL:common>
        1
)
set(common_condition_reference
    "$<$<AND:$<BOOL:common>,1,$<COMPILE_LANGUAGE:CXX>>:-opt>"
    "$<$<AND:$<BOOL:common>,1,$<COMPILE_LANGUAGE:CXX>,$<BOOL:other>>:-opt>"
    "$<$<AND:$<BOOL:common>,1,$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU>>:-opt>"
)

## Check test cases
function(check_options target)
    if(NOT ${target}_reference)
        message(FATAL_ERROR
            "Test case is missing *_reference variable: Case: ${target}"
        )
    endif()
    get_target_property(options ${target} INTERFACE_COMPILE_OPTIONS)
    if(NOT options STREQUAL ${target}_reference)
        message(SEND_ERROR
            "Invalid compile options. Case: ${target}\n"
            "expected-options> ${${target}_reference}\n"
            "actual-options> ${options}"
        )
    endif()
endfunction()

get_directory_property(all_test_cases BUILDSYSTEM_TARGETS)
foreach(case IN LISTS all_test_cases)
    check_options(${case})
endforeach()
