# Generated from test.pro.

#####################################################################
## tst_quuid Test:
#####################################################################

qt_internal_add_test(tst_quuid
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../" # special case
    SOURCES
        ../tst_quuid.cpp
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_quuid CONDITION APPLE
    SOURCES
        ../tst_quuid_darwin.mm
    PUBLIC_LIBRARIES
        ${FWFoundation}
)

#### Keys ignored in scope 4:.:.:test.pro:(CMAKE_BUILD_TYPE STREQUAL Debug):
# DESTDIR = "../debug"

#### Keys ignored in scope 5:.:.:test.pro:else:
# DESTDIR = "../release"

#### Keys ignored in scope 6:.:.:test.pro:else:
# DESTDIR = ".."
