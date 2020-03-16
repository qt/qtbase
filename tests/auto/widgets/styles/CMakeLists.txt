# Generated from styles.pro.

if (MACOS)
    add_subdirectory(qmacstyle)
endif()
add_subdirectory(qstyle)
if (NOT UIKIT AND NOT ANDROID AND NOT QNX)
    add_subdirectory(qstyleoption)
endif()
if (QT_FEATURE_private_tests)
    add_subdirectory(qstylesheetstyle)
endif()
