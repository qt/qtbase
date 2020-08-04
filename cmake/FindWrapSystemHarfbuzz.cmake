# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET WrapSystemHarfbuzz::WrapSystemHarfbuzz)
    set(WrapSystemHarfbuzz_FOUND TRUE)
    return()
endif()
set(WrapSystemHarfbuzz_REQUIRED_VARS __harfbuzz_found)

find_package(harfbuzz ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} QUIET)

# Gentoo has some buggy version of a harfbuzz Config file. Check if include paths are valid.
set(__harfbuzz_target_name "harfbuzz::harfbuzz")
if(harfbuzz_FOUND AND TARGET "${__harfbuzz_target_name}")
    get_property(__harfbuzz_include_paths TARGET "${__harfbuzz_target_name}"
                                          PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    foreach(__harfbuzz_include_dir ${__harfbuzz_include_paths})
        if(NOT EXISTS "${__harfbuzz_include_dir}")
            # Must be the broken Gentoo harfbuzzConfig.cmake file. Try to use pkg-config instead.
            set(__harfbuzz_broken_config_file TRUE)
            break()
        endif()
    endforeach()

    set(__harfbuzz_found TRUE)
    if(harfbuzz_VERSION)
        set(WrapSystemHarfbuzz_VERSION "${harfbuzz_VERSION}")
    endif()
endif()

if(__harfbuzz_broken_config_file OR NOT __harfbuzz_found)
    list(PREPEND WrapSystemHarfbuzz_REQUIRED_VARS HARFBUZZ_LIBRARIES HARFBUZZ_INCLUDE_DIRS)

    find_package(PkgConfig QUIET)
    pkg_check_modules(PC_HARFBUZZ harfbuzz IMPORTED_TARGET)

    find_path(HARFBUZZ_INCLUDE_DIRS
              NAMES harfbuzz/hb.h
              HINTS ${PC_HARFBUZZ_INCLUDEDIR})
    find_library(HARFBUZZ_LIBRARIES
                NAMES harfbuzz
                HINTS ${PC_HARFBUZZ_LIBDIR})

    set(__harfbuzz_target_name "PkgConfig::PC_HARFBUZZ")
    set(__harfbuzz_found TRUE)
    if(PC_HARFBUZZ_VERSION)
        set(WrapSystemHarfbuzz_VERSION "${PC_HARFBUZZ_VERSION}")
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapSystemHarfbuzz
                                  REQUIRED_VARS ${WrapSystemHarfbuzz_REQUIRED_VARS}
                                  VERSION_VAR WrapSystemHarfbuzz_VERSION)
if(WrapSystemHarfbuzz_FOUND)
    add_library(WrapSystemHarfbuzz::WrapSystemHarfbuzz INTERFACE IMPORTED)
    target_link_libraries(WrapSystemHarfbuzz::WrapSystemHarfbuzz
                          INTERFACE "${__harfbuzz_target_name}")
endif()
unset(__harfbuzz_target_name)
unset(__harfbuzz_found)
unset(__harfbuzz_include_dir)
unset(__harfbuzz_broken_config_file)
