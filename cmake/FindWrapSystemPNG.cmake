# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET WrapSystemPNG::WrapSystemPNG)
    set(WrapSystemPNG_FOUND ON)
    return()
endif()

set(WrapSystemPNG_FOUND OFF)

find_package(PNG QUIET)

if(PNG_FOUND)
    set(potential_target_names PNG::PNG)
    foreach(target_name ${potential_target_names})
        if(TARGET ${target_name})
            set(WrapSystemPNG_FOUND ON)
            set(final_target_name ${target_name})

            add_library(WrapSystemPNG::WrapSystemPNG INTERFACE IMPORTED)
            target_link_libraries(WrapSystemPNG::WrapSystemPNG INTERFACE
                                  ${final_target_name})

            break()
        endif()
    endforeach()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapSystemPNG DEFAULT_MSG WrapSystemPNG_FOUND)
