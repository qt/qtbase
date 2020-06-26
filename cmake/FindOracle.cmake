#.rst:
# FindOracle
# ---------
#
# Try to locate the oracle client library.
# If found, this will define the following variables:
#
# ``Oracle_FOUND``
#     True if the oracle library is available
# ``Oracle_INCLUDE_DIRS``
#     The oracle include directories
# ``Oracle_LIBRARIES``
#     The oracle libraries for linking
#
# If ``Oracle_FOUND`` is TRUE, it will also define the following
# imported target:
#
# ``Oracle::Oracle``
#     The oracle instant client library

find_path(Oracle_INCLUDE_DIRS
  NAMES oci.h
  HINTS ${Oracle_INCLUDE_DIR})

set(ORACLE_OCI_NAMES clntsh ociei oraociei12)

find_library(Oracle_LIBRARIES
  NAMES NAMES ${ORACLE_OCI_NAMES}
  HINTS ${Oracle_LIBRARY_DIR})

if (NOT Oracle_INCLUDE_DIRS STREQUAL "Oracle_INCLUDE_DIRS-NOTFOUND" AND NOT Oracle_LIBRARIES STREQUAL "Oracle_LIBRARIES-NOTFOUND")
  set(Oracle_FOUND ON)
endif()

if(Oracle_FOUND AND NOT TARGET Oracle::OCI)
  add_library(Oracle::OCI UNKNOWN IMPORTED)
  set_target_properties(Oracle::OCI PROPERTIES
                        IMPORTED_LOCATION "${Oracle_LIBRARIES}"
                        INTERFACE_INCLUDE_DIRECTORIES "${Oracle_INCLUDE_DIRS}")
endif()

mark_as_advanced(Oracle_INCLUDE_DIRS Oracle_LIBRARIES)

include(FeatureSummary)
set_package_properties(Oracle PROPERTIES
  URL "https://www.oracle.com"
  DESCRIPTION "Oracle client library")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Oracle DEFAULT_MSG Oracle_INCLUDE_DIRS Oracle_LIBRARIES)
