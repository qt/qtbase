#####################################################################
## tst_quuid Test:
#####################################################################

qt_internal_add_test(tst_quuid
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    SOURCES
        ../tst_quuid.cpp
)

## Scopes:

qt_internal_extend_target(tst_quuid CONDITION APPLE
    SOURCES
        ../tst_quuid_darwin.mm
    PUBLIC_LIBRARIES
        ${FWFoundation}
)

if(QT_FEATURE_process AND NOT ANDROID)
    add_dependencies(tst_quuid testProcessUniqueness)
endif()
