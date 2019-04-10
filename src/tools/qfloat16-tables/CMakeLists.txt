# Generated from qfloat16-tables.pro.

#####################################################################
## qfloat16-tables Tool:
#####################################################################

add_qt_tool(qfloat16-tables
    BOOTSTRAP
    TOOLS_TARGET Core # special case
    SOURCES
        gen_qfloat16_tables.cpp
    # CONFIG = "force_bootstrap"
    # INSTALLS = "lib"
    # _LOADED = "qt_tool"
    # _OPTION = "host_build"
    # lib.CONFIG = "dummy_install"
)
