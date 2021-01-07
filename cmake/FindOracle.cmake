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

find_path(Oracle_INCLUDE_DIR
  NAMES oci.h
  HINTS ${Oracle_INCLUDE_DIR})

set(ORACLE_OCI_NAMES clntsh ociei oraociei12 oci)

find_library(Oracle_LIBRARY
  NAMES ${ORACLE_OCI_NAMES}
  HINTS ${Oracle_LIBRARY_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Oracle DEFAULT_MSG Oracle_LIBRARY Oracle_INCLUDE_DIR)

if(Oracle_FOUND)
  set(Oracle_INCLUDE_DIRS "${Oracle_INCLUDE_DIR}")
  set(Oracle_LIBRARIES "${Oracle_LIBRARY}")
  if(NOT TARGET Oracle::OCI)
    add_library(Oracle::OCI UNKNOWN IMPORTED)
    set_target_properties(Oracle::OCI PROPERTIES
                        IMPORTED_LOCATION "${Oracle_LIBRARIES}"
                        INTERFACE_INCLUDE_DIRECTORIES "${Oracle_INCLUDE_DIRS}")
  endif()
endif()

mark_as_advanced(Oracle_INCLUDE_DIR Oracle_LIBRARY)

include(FeatureSummary)
set_package_properties(Oracle PROPERTIES
  URL "https://www.oracle.com"
  DESCRIPTION "Oracle client library")
