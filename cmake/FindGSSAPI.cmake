find_package(PkgConfig QUIET)
pkg_check_modules(PC_GSSAPI QUIET krb5-gssapi)
if (NOT PC_GSSAPI_FOUND)
    pkg_check_modules(PC_GSSAPI QUIET mit-krb5-gssapi)
endif()

find_path(GSSAPI_INCLUDE_DIRS
          NAMES gssapi/gssapi.h
          HINTS ${PC_GSSAPI_INCLUDEDIR}
)

find_library(GSSAPI_LIBRARIES
             NAMES
             GSS # framework
             gss # solaris
             gssapi_krb5
             HINTS ${PC_GSSAPI_LIBDIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GSSAPI DEFAULT_MSG GSSAPI_LIBRARIES GSSAPI_INCLUDE_DIRS)

if(GSSAPI_FOUND AND NOT TARGET GSSAPI::GSSAPI)
    if(GSSAPI_LIBRARIES MATCHES "/([^/]+)\\.framework$")
        add_library(GSSAPI::GSSAPI INTERFACE IMPORTED)
        set_target_properties(GSSAPI::GSSAPI PROPERTIES
                              INTERFACE_LINK_LIBRARIES "${GSSAPI_LIBRARIES}")
    else()
      add_library(GSSAPI::GSSAPI UNKNOWN IMPORTED)
      set_target_properties(GSSAPI::GSSAPI PROPERTIES
                            IMPORTED_LOCATION "${GSSAPI_LIBRARIES}")
    endif()

    set_target_properties(GSSAPI::GSSAPI PROPERTIES
                          INTERFACE_INCLUDE_DIRECTORIES "${GSSAPI_INCLUDE_DIRS}")
endif()

mark_as_advanced(GSSAPI_INCLUDE_DIRS GSSAPI_LIBRARIES)

include(FeatureSummary)
set_package_properties(GSSAPI PROPERTIES
  DESCRIPTION "Generic Security Services Application Program Interface")

