# Generated from qtcpsocket.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()
add_subdirectory(test)
if(NOT VXWORKS)
    add_subdirectory(stressTest)
endif()
