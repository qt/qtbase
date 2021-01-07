#.rst:
# FindMySQL
# ---------
#
# Try to locate the mysql client library.
# If found, this will define the following variables:
#
# ``MySQL_FOUND``
#     True if the mysql library is available
# ``MySQL_INCLUDE_DIRS``
#     The mysql include directories
# ``MySQL_LIBRARIES``
#     The mysql libraries for linking
#
# If ``MySQL_FOUND`` is TRUE, it will also define the following
# imported target:
#
# ``MySQL::MySQL``
#     The mysql client library

find_package(PkgConfig QUIET)
pkg_check_modules(PC_MySQL QUIET mysqlclient)

find_path(MySQL_INCLUDE_DIR
          NAMES mysql.h
          HINTS ${PC_MySQL_INCLUDEDIR}
          PATH_SUFFIXES mysql mariadb)

find_library(MySQL_LIBRARY
             NAMES libmysql mysql mysqlclient libmariadb mariadb
             HINTS ${PC_MySQL_LIBDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MySQL DEFAULT_MSG MySQL_LIBRARY MySQL_INCLUDE_DIR)

if(MySQL_FOUND)
  set(MySQL_INCLUDE_DIRS "${MySQL_INCLUDE_DIR}")
  set(MySQL_LIBRARIES "${MySQL_LIBRARY}")
  if(NOT TARGET MySQL::MySQL)
    add_library(MySQL::MySQL UNKNOWN IMPORTED)
    set_target_properties(MySQL::MySQL PROPERTIES
                          IMPORTED_LOCATION "${MySQL_LIBRARIES}"
                          INTERFACE_INCLUDE_DIRECTORIES "${MySQL_INCLUDE_DIRS}")
  endif()
endif()

mark_as_advanced(MySQL_INCLUDE_DIR MySQL_LIBRARY)

include(FeatureSummary)
set_package_properties(MySQL PROPERTIES
  URL "https://www.mysql.com"
  DESCRIPTION "MySQL client library")

