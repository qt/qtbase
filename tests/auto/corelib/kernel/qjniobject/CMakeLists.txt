#####################################################################
## tst_qjniobject Test:
#####################################################################

qt_internal_add_test(tst_qjniobject
    SOURCES
        tst_qjniobject.cpp
)

if(ANDROID)
    set_property(TARGET tst_qjniobject APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
        ${CMAKE_CURRENT_SOURCE_DIR}/testdata
    )
    # QTBUG-88840 # special case
    qt_android_generate_deployment_settings(tst_qjniobject) # special case
endif()
