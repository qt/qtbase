# Generated from android.pro.

#####################################################################
## tst_android Test:
#####################################################################

qt_internal_add_test(tst_android
    SOURCES
        tst_android.cpp
)

if(ANDROID)
    set_property(TARGET tst_android APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
        ${CMAKE_CURRENT_SOURCE_DIR}/testdata
    )
    # QTBUG-88840 # special case
    qt_android_generate_deployment_settings(tst_android) # special case
endif()

#### Keys ignored in scope 1:.:.:android.pro:<TRUE>:
# DISTFILES = "testdata/assets/test.txt"
