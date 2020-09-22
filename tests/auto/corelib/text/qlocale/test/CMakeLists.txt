# Generated from test.pro.

#####################################################################
## tst_qlocale Test:
#####################################################################

qt_internal_add_test(tst_qlocale
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    SOURCES
        ../tst_qlocale.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qlocale CONDITION embedded
    PUBLIC_LIBRARIES
        Qt::Gui
)

qt_internal_extend_target(tst_qlocale CONDITION NOT QT_FEATURE_doubleconversion AND NOT QT_FEATURE_system_doubleconversion
    DEFINES
        QT_NO_DOUBLECONVERSION
)

#### Keys ignored in scope 7:.:.:test.pro:NOT ANDROID:
# TEST_HELPER_INSTALLS = "../syslocaleapp/syslocaleapp"
