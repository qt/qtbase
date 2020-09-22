# Generated from fromiccprofile.pro.

#####################################################################
## fromiccprofile Binary:
#####################################################################

qt_internal_add_executable(fromiccprofile
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)

#### Keys ignored in scope 1:.:.:fromiccprofile.pro:<TRUE>:
# FUZZ_ENGINE = "$$(LIB_FUZZING_ENGINE)"
# QTPLUGIN = "qminimal"

## Scopes:
#####################################################################

qt_internal_extend_target(fromiccprofile CONDITION FUZZ_ENGINE_ISEMPTY
    LINK_OPTIONS
        "-fsanitize=fuzzer"
)

qt_internal_extend_target(fromiccprofile CONDITION NOT FUZZ_ENGINE_ISEMPTY
    PUBLIC_LIBRARIES
        $ENV{LIB_FUZZING_ENGINE}
)
