# Generated from beginlayout.pro.

#####################################################################
## beginlayout Binary:
#####################################################################

qt_internal_add_executable(beginlayout
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)

#### Keys ignored in scope 1:.:.:beginlayout.pro:<TRUE>:
# FUZZ_ENGINE = "$$(LIB_FUZZING_ENGINE)"
# QTPLUGIN = "qminimal"

## Scopes:
#####################################################################

qt_internal_extend_target(beginlayout CONDITION FUZZ_ENGINE_ISEMPTY
    LINK_OPTIONS
        "-fsanitize=fuzzer"
)

qt_internal_extend_target(beginlayout CONDITION NOT FUZZ_ENGINE_ISEMPTY
    PUBLIC_LIBRARIES
        $ENV{LIB_FUZZING_ENGINE}
)
