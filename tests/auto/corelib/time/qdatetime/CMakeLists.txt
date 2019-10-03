# Generated from qdatetime.pro.

#####################################################################
## tst_qdatetime Test:
#####################################################################

add_qt_test(tst_qdatetime
    SOURCES
        tst_qdatetime.cpp
    LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qdatetime.pro:MSVC:
# QMAKE_CFLAGS_RELEASE = "--O1"
# QMAKE_CXXFLAGS_RELEASE = "--O1"

extend_target(tst_qdatetime CONDITION APPLE
    SOURCES
        tst_qdatetime_mac.mm
    PUBLIC_LIBRARIES
        ${FWFoundation}
)
