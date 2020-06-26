# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET WrapDoubleConversion::WrapDoubleConversion)
    set(WrapDoubleConversion_FOUND ON)
    return()
endif()

set(WrapDoubleConversion_FOUND OFF)

find_package(double-conversion QUIET)
if (double-conversion_FOUND)
    include(FeatureSummary)
    set_package_properties(double-conversion PROPERTIES TYPE REQUIRED)
    add_library(WrapDoubleConversion::WrapDoubleConversion INTERFACE IMPORTED)
    target_link_libraries(WrapDoubleConversion::WrapDoubleConversion
                          INTERFACE double-conversion::double-conversion)
    set(WrapDoubleConversion_FOUND ON)
    return()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapDoubleConversion DEFAULT_MSG WrapDoubleConversion_FOUND)
