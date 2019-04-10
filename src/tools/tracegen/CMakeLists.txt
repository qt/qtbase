# Generated from tracegen.pro.

#####################################################################
## tracegen Tool:
#####################################################################

add_qt_tool(tracegen
    BOOTSTRAP
    TOOLS_TARGET Core # special case
    SOURCES
        etw.cpp etw.h
        helpers.cpp helpers.h
        lttng.cpp lttng.h
        panic.cpp panic.h
        provider.cpp provider.h
        qtheaders.cpp qtheaders.h
        tracegen.cpp
)

#### Keys ignored in scope 1:.:.:tracegen.pro:<TRUE>:
# _LOADED = "qt_tool"
# _OPTION = "host_build"
