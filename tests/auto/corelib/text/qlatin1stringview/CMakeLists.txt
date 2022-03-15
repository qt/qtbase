#####################################################################
## tst_qlatin1stringview Test:
#####################################################################

qt_internal_add_test(tst_qlatin1stringview
    SOURCES
        tst_qlatin1stringview.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
        QT_NO_CAST_TO_ASCII
)

## Scopes:
#####################################################################
