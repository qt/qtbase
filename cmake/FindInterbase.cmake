#.rst:
# FindInterbase
# ---------
#
# Try to locate the Interbase client library.
# If found, this will define the following variables:
#
# ``Interbase_FOUND``
#     True if the Interbase library is available
# ``Interbase_INCLUDE_DIR``
#     The Interbase include directories
# ``Interbase_LIBRARY``
#     The Interbase libraries for linking
#
# If ``Interbase_FOUND`` is TRUE, it will also define the following
# imported target:
#
# ``Interbase::Interbase``
#     The Interbase client library

find_path(Interbase_INCLUDE_DIR
          NAMES ibase.h
          HINTS ${Interbase_INCLUDEDIR}
)

find_library(Interbase_LIBRARY
             NAMES firebase_ms fbclient gds
             HINTS ${Interbase_LIBDIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Interbase DEFAULT_MSG Interbase_LIBRARY Interbase_INCLUDE_DIR)

if(Interbase_FOUND)
  set(Interbase_INCLUDE_DIRS "${Interbase_INCLUDE_DIR}")
  set(Interbase_LIBRARIES "${Interbase_LIBRARY}")
  if(NOT TARGET Interbase::Interbase)
    add_library(Interbase::Interbase UNKNOWN IMPORTED)
    set_target_properties(Interbase::Interbase PROPERTIES
                          IMPORTED_LOCATION "${Interbase_LIBRARIES}"
                          INTERFACE_INCLUDE_DIRECTORIES "${Interbase_INCLUDE_DIRS};")
  endif()
endif()

mark_as_advanced(Interbase_INCLUDE_DIR Interbase_LIBRARY)

include(FeatureSummary)
set_package_properties(Interbase PROPERTIES
  URL "https://www.embarcadero.com/products/interbase"
  DESCRIPTION "Interbase client library")

