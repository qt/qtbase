add_library(dummy INTERFACE)
message(STATUS "case: ${case}")
if(case STREQUAL "missing_OPTIONS")
    qt_internal_add_compiler_dependent_flags(dummy INTERFACE
        COMPILERS ALL
    )
elseif(case STREQUAL "missing_COMPILERS")
    qt_internal_add_compiler_dependent_flags(dummy INTERFACE
            CONDITIONS $<BOOL:True>
                OPTIONS -foo
    )
elseif(case STREQUAL "empty_COMPILERS")
    qt_internal_add_compiler_dependent_flags(dummy INTERFACE
        COMPILERS
                OPTIONS -foo
    )
elseif(case STREQUAL "invalid_CONDITIONS")
    qt_internal_add_compiler_dependent_flags(dummy INTERFACE
        COMPILERS ALL
            CONDITIONS not_a_genex
                OPTIONS -foo
    )
else()
    message(FATAL_ERROR "Unrecognized case: ${case}")
endif()
