# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET WrapZLIB::WrapZLIB)
    set(WrapZLIB_FOUND ON)
    return()
endif()

set(WrapZLIB_FOUND OFF)

find_package(ZLIB ${WrapZLIB_FIND_VERSION})

if(ZLIB_FOUND)
    set(WrapZLIB_FOUND ON)

    add_library(WrapZLIB::WrapZLIB INTERFACE IMPORTED)
    if(APPLE)
        # On Darwin platforms FindZLIB sets IMPORTED_LOCATION to the absolute path of the library
        # within the framework. This ends up as an absolute path link flag, which we don't want,
        # because that makes our .prl files un-relocatable and also breaks iOS simulator_and_device
        # SDK switching in Xcode.
        # Just pass a linker flag instead.
        target_link_libraries(WrapZLIB::WrapZLIB INTERFACE "-lz")
    else()
        target_link_libraries(WrapZLIB::WrapZLIB INTERFACE ZLIB::ZLIB)
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapZLIB DEFAULT_MSG WrapZLIB_FOUND)
