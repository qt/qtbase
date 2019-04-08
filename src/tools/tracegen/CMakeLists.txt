# Generated from tracegen.pro.

#####################################################################
## tracegen Tool:
#####################################################################

add_qt_tool(tracegen
    BOOTSTRAP
    SOURCES
        etw.cpp etw.h
        helpers.cpp helpers.h
        lttng.cpp lttng.h
        panic.cpp panic.h
        provider.cpp provider.h
        qtheaders.cpp qtheaders.h
        tracegen.cpp
    # CONFIG = "force_bootstrap"
    # _LOADED = "qt_tool"
    # _OPTION = "host_build"
)
