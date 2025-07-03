include(${CMAKE_CURRENT_LIST_DIR}/3rdparty/cmake/QtRunCMakeTestHelpers.cmake)

if(CMAKE_VERSION VERSION_LESS 3.17.0)
    set(CMAKE_CURRENT_FUNCTION_LIST_DIR "${CMAKE_CURRENT_LIST_DIR}")
endif()

function(qt_internal_add_RunCMake_test test)
    # Add the common Qt specific setups
    set(common_args
        "-DCMAKE_MODULE_PATH=${CMAKE_CURRENT_FUNCTION_LIST_DIR}"
    )
    add_RunCMake_test(${test} ${common_args} ${ARGN})
endfunction()
