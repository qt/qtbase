#####################################################################
## tst_qjnienvironment Test:
#####################################################################

qt_internal_add_test(tst_qjnienvironment
    SOURCES
        tst_qjnienvironment.cpp
)

if(ANDROID)
    set_property(TARGET tst_qjnienvironment APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
        ${CMAKE_CURRENT_SOURCE_DIR}/testdata
    )
    # QTBUG-88840
    qt_android_generate_deployment_settings(tst_qjnienvironment)
endif()
