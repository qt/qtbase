# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET WrapFreetype::WrapFreetype)
    set(WrapFreetype_FOUND ON)
    return()
endif()

set(WrapFreetype_FOUND OFF)

# Hunter has the package named freetype, but exports the Freetype::Freetype target as upstream
# First try the CONFIG package, and afterwards the MODULE if not found

find_package(Freetype CONFIG NAMES Freetype freetype QUIET)
if(NOT Freetype_FOUND)
    find_package(Freetype MODULE)
endif()

if(Freetype_FOUND)
    # vcpkg defines a lower case target name, while upstream Find module defines a prefixed
    # upper case name.
    set(potential_target_names Freetype::Freetype freetype)
    foreach(target_name ${potential_target_names})
        if(TARGET ${target_name})
            set(WrapFreetype_FOUND ON)
            set(final_target_name ${target_name})

            add_library(WrapFreetype::WrapFreetype INTERFACE IMPORTED)
            target_link_libraries(WrapFreetype::WrapFreetype INTERFACE ${final_target_name})

            break()
        endif()
    endforeach()
endif()
