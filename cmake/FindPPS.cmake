# Find the PPS library

# Will make the target PPS::PPS available when found.
if(TARGET PPS::PPS)
    set(PPS_FOUND TRUE)
    return()
endif()

find_library(PPS_LIBRARY NAMES "pps")
find_path(PPS_INCLUDE_DIR NAMES "sys/pps.h" DOC "The PPS Include path")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PPS DEFAULT_MSG PPS_INCLUDE_DIR PPS_LIBRARY)

mark_as_advanced(PPS_INCLUDE_DIR PPS_LIBRARY)

if(PPS_FOUND)
    add_library(__PPS INTERFACE IMPORTED)
    target_link_libraries(__PPS INTERFACE ${PPS_LIBRARY})
    target_include_directories(__PPS INTERFACE ${PPS_INCLUDE_DIR})

    add_library(PPS::PPS ALIAS __PPS)
endif()
