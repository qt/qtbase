# Generated from test.pro.

#####################################################################
## test Test:
#####################################################################

# Collect test data
list(APPEND test_data "../tmp/README")
list(APPEND test_data "../modal")

qt_internal_add_test(tst_qapplication # special case
    SOURCES
        ../tst_qapplication.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
    TESTDATA ${test_data}
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.." # special case
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qapplication CONDITION builtin_testdata # special case
    DEFINES
        BUILTIN_TESTDATA
)

#### Keys ignored in scope 3:.:.:test.pro:NOT ANDROID:
# SUBPROGRAMS = "desktopsettingsaware" "modal"

#### Keys ignored in scope 6:.:.:test.pro:NOT ANDROID:
# TEST_HELPER_INSTALLS = "../debug/helper"

#### Keys ignored in scope 8:.:.:test.pro:NOT ANDROID:
# TEST_HELPER_INSTALLS = "../release/helper"

#### Keys ignored in scope 10:.:.:test.pro:NOT ANDROID:
# TEST_HELPER_INSTALLS = "../helper"
