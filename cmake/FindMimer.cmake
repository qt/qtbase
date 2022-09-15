# Copyright (C) 2022 The Qt Company Ltd.
# Copyright (C) 2022 Mimer Information Technology
# SPDX-License-Identifier: BSD-3-Clause

# FindMimer
# ---------
# Try to locate the Mimer SQL client library

find_package(PkgConfig QUIET)
pkg_check_modules(PC_Mimer QUIET mimctrl)

find_path(Mimer_INCLUDE_DIR
    NAMES mimerapi.h
    HINTS ${PC_Mimer_INCLUDEDIR})

if(WIN32)
  if("$ENV{PROCESSOR_ARCHITECTURE}" STREQUAL "x86")
    set(MIMER_LIBS_NAMES mimapi32)
  else()
    set(MIMER_LIBS_NAMES mimapi64)
  endif()
else()
  set(MIMER_LIBS_NAMES mimerapi)
endif()

find_library(Mimer_LIBRARIES
    NAMES ${MIMER_LIBS_NAMES}
    HINTS ${PC_Mimer_LIBDIR})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Mimer
    REQUIRED_VARS Mimer_LIBRARIES Mimer_INCLUDE_DIR)



# Now try to get the include and library path.
if(Mimer_FOUND)
  set(Mimer_INCLUDE_DIRS ${Mimer_INCLUDE_DIR})
  set(Mimer_LIBRARY_DIRS ${Mimer_LIBRARIES})
  if (NOT TARGET MimerSQL::MimerSQL)
    add_library(MimerSQL::MimerSQL UNKNOWN IMPORTED)
    set_target_properties(MimerSQL::MimerSQL PROPERTIES
      IMPORTED_LOCATION "${Mimer_LIBRARY_DIRS}"
      INTERFACE_INCLUDE_DIRECTORIES "${Mimer_INCLUDE_DIRS}")
  endif ()
endif()

mark_as_advanced(Mimer_INCLUDE_DIR Mimer_LIBRARIES)

include(FeatureSummary)
set_package_properties(MimerSQL PROPERTIES
  URL "https://www.mimer.com"
  DESCRIPTION "Mimer client library")
