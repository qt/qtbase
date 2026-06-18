# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#.rst:
# FindFirebird
# ------------
#
# Try to locate the Firebird client library and its modern object-oriented
# C++ API (firebird/Interface.h, Firebird 4.0 and later).
# If found, this will define the following variables:
#
# ``Firebird_FOUND``
#     True if the Firebird client library is available
# ``Firebird_INCLUDE_DIRS``
#     The Firebird include directories
# ``Firebird_LIBRARIES``
#     The Firebird libraries for linking
# ``Firebird_VERSION``
#     The Firebird client API version as a dotted string (e.g. "4.0", "5.0")
# ``Firebird_API_VERSION``
#     The raw Firebird client API level (FB_API_VER: 30, 40, 50, ...)
#
# Input variables:
#
# ``Firebird_MINIMUM_API_VERSION``
#     Optional. The minimum required client API level as the raw FB_API_VER
#     value (e.g. 40 for Firebird 4.0, 50 for 5.0). If set, the package is
#     reported as not found when an older client is detected; leaving it unset
#     imposes no minimum. The standard find_package(Firebird <major.minor>)
#     version argument is also honoured (via Firebird_VERSION).
#
# If ``Firebird_FOUND`` is TRUE, it will also define the following
# imported target:
#
# ``Firebird::Client``
#     The Firebird client library
#
# The location can be hinted by setting ``Firebird_ROOT``, the ``FIREBIRD``
# environment variable (Firebird's own convention), or the CMake cache
# variables ``Firebird_INCLUDE_DIR`` and ``Firebird_LIBRARY`` directly.

if(NOT DEFINED Firebird_ROOT)
    if(DEFINED ENV{Firebird_ROOT})
        set(Firebird_ROOT "$ENV{Firebird_ROOT}")
    elseif(DEFINED ENV{FIREBIRD})
        set(Firebird_ROOT "$ENV{FIREBIRD}")
    endif()
endif()

find_path(Firebird_INCLUDE_DIR
          NAMES firebird/Interface.h
          HINTS "${Firebird_INCLUDEDIR}" "${Firebird_ROOT}/include" "${Firebird_ROOT}"
          PATH_SUFFIXES include
)

find_library(Firebird_LIBRARY
             NAMES fbclient fbclient_ms
             HINTS "${Firebird_LIBDIR}" "${Firebird_ROOT}/lib" "${Firebird_ROOT}"
             PATH_SUFFIXES lib
)

# Detect the client API level from FB_API_VER in <firebird/ibase.h>
# (30 = Firebird 3.0, 40 = 4.0, 50 = 5.0).
if(Firebird_INCLUDE_DIR AND EXISTS "${Firebird_INCLUDE_DIR}/firebird/ibase.h")
    file(STRINGS "${Firebird_INCLUDE_DIR}/firebird/ibase.h" _fb_api_ver_line
         REGEX "^#define[ \t]+FB_API_VER[ \t]+[0-9]+")
    if(_fb_api_ver_line MATCHES "FB_API_VER[ \t]+([0-9]+)")
        set(Firebird_API_VERSION "${CMAKE_MATCH_1}")
        math(EXPR _fb_major "${Firebird_API_VERSION} / 10")
        math(EXPR _fb_minor "${Firebird_API_VERSION} % 10")
        set(Firebird_VERSION "${_fb_major}.${_fb_minor}")
    endif()
    unset(_fb_api_ver_line)
endif()

# Honour the optional minimum client API level (raw FB_API_VER); unset = no minimum.
if(NOT DEFINED Firebird_MINIMUM_API_VERSION
        OR (Firebird_API_VERSION AND NOT Firebird_API_VERSION LESS Firebird_MINIMUM_API_VERSION))
    set(Firebird_API_VERSION_OK TRUE)
else()
    set(Firebird_API_VERSION_OK FALSE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Firebird
    REQUIRED_VARS Firebird_LIBRARY Firebird_INCLUDE_DIR Firebird_API_VERSION_OK
    VERSION_VAR Firebird_VERSION)

if(Firebird_FOUND)
  set(Firebird_INCLUDE_DIRS "${Firebird_INCLUDE_DIR}")
  set(Firebird_LIBRARIES "${Firebird_LIBRARY}")
  if(NOT TARGET Firebird::Client)
    add_library(Firebird::Client UNKNOWN IMPORTED)
    set_target_properties(Firebird::Client PROPERTIES
                          IMPORTED_LOCATION "${Firebird_LIBRARIES}"
                          INTERFACE_INCLUDE_DIRECTORIES "${Firebird_INCLUDE_DIRS}")
  endif()
endif()

mark_as_advanced(Firebird_INCLUDE_DIR Firebird_LIBRARY)

include(FeatureSummary)
set_package_properties(Firebird PROPERTIES
  URL "https://firebirdsql.org/"
  DESCRIPTION "Firebird client library")
