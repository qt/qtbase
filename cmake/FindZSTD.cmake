#.rst:
# FindZstd
# ---------
#
# Try to locate the Zstd library.
# If found, this will define the following variables:
#
# ``ZSTD_FOUND``
#     True if the zstd library is available
# ``ZSTD_INCLUDE_DIRS``
#     The zstd include directories
# ``ZSTD_LIBRARIES``
#     The zstd libraries for linking
#
# If ``ZSTD_FOUND`` is TRUE, it will also define the following
# imported target:
#
# ``ZSTD::ZSTD``
#     The zstd library

find_package(zstd CONFIG QUIET)

include(FindPackageHandleStandardArgs)

if(TARGET zstd::libzstd_static OR TARGET zstd::libzstd_shared)
    find_package_handle_standard_args(ZSTD REQUIRED_VARS zstd_VERSION VERSION_VAR zstd_VERSION)
    if(TARGET zstd::libzstd_static)
        set(zstdtargetsuffix "_static")
    else()
        set(zstdtargetsuffix "_shared")
    endif()
    if(NOT TARGET ZSTD::ZSTD)
        add_library(ZSTD::ZSTD INTERFACE IMPORTED)
        set_target_properties(ZSTD::ZSTD PROPERTIES
                              INTERFACE_LINK_LIBRARIES "zstd::libzstd${zstdtargetsuffix}")
    endif()
else()
    find_package(PkgConfig QUIET)
    pkg_check_modules(PC_ZSTD QUIET libzstd)

    find_path(ZSTD_INCLUDE_DIRS
              NAMES zstd.h
              HINTS ${PC_ZSTD_INCLUDEDIR}
              PATH_SUFFIXES zstd)

    find_library(ZSTD_LIBRARY_RELEASE
                 NAMES zstd zstd_static
                 HINTS ${PC_ZSTD_LIBDIR}
    )
    find_library(ZSTD_LIBRARY_DEBUG
                 NAMES zstdd zstd_staticd zstd zstd_static
                 HINTS ${PC_ZSTD_LIBDIR}
    )

    include(SelectLibraryConfigurations)
    select_library_configurations(ZSTD)

    find_package_handle_standard_args(ZSTD REQUIRED_VARS ZSTD_LIBRARIES ZSTD_INCLUDE_DIRS
                                           VERSION_VAR PC_ZSTD_VERSION)

    if(ZSTD_FOUND AND NOT TARGET ZSTD::ZSTD)
      add_library(ZSTD::ZSTD UNKNOWN IMPORTED)
      set_target_properties(ZSTD::ZSTD PROPERTIES
                            INTERFACE_INCLUDE_DIRECTORIES "${ZSTD_INCLUDE_DIRS}")
      set_target_properties(ZSTD::ZSTD PROPERTIES
                            IMPORTED_LOCATION "${ZSTD_LIBRARY}")
      if(ZSTD_LIBRARY_RELEASE)
          set_target_properties(ZSTD::ZSTD PROPERTIES
                                IMPORTED_LOCATION_RELEASE "${ZSTD_LIBRARY_RELEASE}")
      endif()
      if(ZSTD_LIBRARY_DEBUG)
          set_target_properties(ZSTD::ZSTD PROPERTIES
                                IMPORTED_LOCATION_DEBUG "${ZSTD_LIBRARY_DEBUG}")
      endif()
    endif()

    mark_as_advanced(ZSTD_INCLUDE_DIRS ZSTD_LIBRARIES ZSTD_LIBRARY_RELEASE ZSTD_LIBRARY_DEBUG)
endif()
include(FeatureSummary)
set_package_properties(ZSTD PROPERTIES
  URL "https://github.com/facebook/zstd"
  DESCRIPTION "ZSTD compression library")

