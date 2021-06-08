# Generated from qtemporarydir.pro.

#####################################################################
## tst_qtemporarydir Test:
#####################################################################

qt_internal_add_test(tst_qtemporarydir
    SOURCES
        tst_qtemporarydir.cpp
    PUBLIC_LIBRARIES
        Qt::TestPrivate
)

qt_internal_extend_target(tst_qtemporarydir CONDITION WIN32
    LIBRARIES
        shlwapi
)
