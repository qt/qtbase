# Generated from styles.pro.

if(APPLE)
    add_subdirectory(qmacstyle)
endif()
add_subdirectory(qstyle)
add_subdirectory(qstyleoption)
if(QT_FEATURE_private_tests AND NOT ANDROID AND NOT QNX AND NOT UIKIT)
    add_subdirectory(qstylesheetstyle)
endif()
