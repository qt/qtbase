# Generated from qline.pro.

#####################################################################
## tst_qline Test:
#####################################################################

qt_internal_add_test(tst_qline
    SOURCES
        tst_qline.cpp
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qline CONDITION UNIX AND NOT APPLE AND NOT HAIKU AND NOT INTEGRITY AND NOT VXWORKS
    PUBLIC_LIBRARIES
        m
)
