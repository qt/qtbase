# Generated from qtcpsocket.pro.

add_subdirectory(test)

if(NOT VXWORKS)
    add_subdirectory(stressTest)
endif()
