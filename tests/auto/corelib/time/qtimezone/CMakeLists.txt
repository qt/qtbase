# Generated from qtimezone.pro.

#####################################################################
## tst_qtimezone Test:
#####################################################################

add_qt_test(tst_qtimezone
    SOURCES
        tst_qtimezone.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################

extend_target(tst_qtimezone CONDITION QT_FEATURE_icu
    LIBRARIES
        ICU::i18n ICU::uc ICU::data
)

extend_target(tst_qtimezone CONDITION APPLE
    SOURCES
        tst_qtimezone_darwin.mm
    PUBLIC_LIBRARIES
        ${FWFoundation}
)
