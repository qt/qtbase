# Generated from optimize.pro.

#####################################################################
## optimize Binary:
#####################################################################

qt_internal_add_executable(optimize
    GUI
    SOURCES
        main.cpp
)

#### Keys ignored in scope 1:.:.:optimize.pro:<TRUE>:
# FUZZ_ENGINE = "$$(LIB_FUZZING_ENGINE)"

## Scopes:
#####################################################################

qt_internal_extend_target(optimize CONDITION FUZZ_ENGINE_ISEMPTY
    LINK_OPTIONS
        "-fsanitize=fuzzer"
)

qt_internal_extend_target(optimize CONDITION NOT FUZZ_ENGINE_ISEMPTY
    PUBLIC_LIBRARIES
        $ENV{LIB_FUZZING_ENGINE}
)
