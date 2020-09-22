# Generated from qstring.pro.

#####################################################################
## tst_qstring Test:
#####################################################################

qt_internal_add_test(tst_qstring
    SOURCES
        tst_qstring.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qstring CONDITION NOT QT_FEATURE_doubleconversion AND NOT QT_FEATURE_system_doubleconversion
    DEFINES
        QT_NO_DOUBLECONVERSION
)

qt_internal_extend_target(tst_qstring CONDITION APPLE
    SOURCES
        tst_qstring_mac.mm
    PUBLIC_LIBRARIES
        ${FWFoundation}
)
