#.rst:
# FindGLIB2
# ---------
#
# Try to locate the GLib2 library.
# If found, this will define the following variables:
#
# ``GLIB2_FOUND``
#     True if the GLib2 library is available
# ``GLIB2_INCLUDE_DIRS``
#     The GLib2 include directories
# ``GLIB2_LIBRARIES``
#     The GLib2 libraries for linking
# ``GLIB2_INCLUDE_DIR``
#     Deprecated, use ``GLIB2_INCLUDE_DIRS``
# ``GLIB2_LIBRARY``
#     Deprecated, use ``GLIB2_LIBRARIES``
#
# If ``GLIB2_FOUND`` is TRUE, it will also define the following
# imported target:
#
# ``GLIB2::GLIB2``
#     The GLIB2 library
#
# Since 5.41.0.

#=============================================================================
# Copyright (c) 2008 Laurent Montel, <montel@kde.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

find_package(PkgConfig QUIET)
pkg_check_modules(PC_GLIB2 QUIET glib-2.0)

find_path(GLIB2_INCLUDE_DIRS
          NAMES glib.h
          HINTS ${PC_GLIB2_INCLUDEDIR}
          PATH_SUFFIXES glib-2.0)

find_library(GLIB2_LIBRARIES
             NAMES glib-2.0
             HINTS ${PC_GLIB2_LIBDIR}
)

pkg_check_modules(PC_GTHREAD2 QUIET gthread-2.0)

find_library(GTHREAD2_LIBRARIES
             NAMES gthread-2.0
             HINTS ${PC_GTHREAD2_LIBDIR}
)

# search the glibconfig.h include dir under the same root where the library is found
get_filename_component(glib2LibDir "${GLIB2_LIBRARIES}" PATH)

find_path(GLIB2_INTERNAL_INCLUDE_DIR glibconfig.h
          PATH_SUFFIXES glib-2.0/include
          HINTS ${PC_GLIB2_INCLUDEDIR} "${glib2LibDir}" ${CMAKE_SYSTEM_LIBRARY_PATH})

# not sure if this include dir is optional or required
# for now it is optional
if(GLIB2_INTERNAL_INCLUDE_DIR)
  list(APPEND GLIB2_INCLUDE_DIRS "${GLIB2_INTERNAL_INCLUDE_DIR}")
endif()

# Deprecated synonyms
set(GLIB2_INCLUDE_DIR "${GLIB2_INCLUDE_DIRS}")
set(GLIB2_LIBRARY "${GLIB2_LIBRARIES}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLIB2 DEFAULT_MSG GLIB2_LIBRARIES GTHREAD2_LIBRARIES GLIB2_INCLUDE_DIRS)

if(GLIB2_FOUND AND NOT TARGET GLIB2::GLIB2)
  add_library(GLIB2::GLIB2 UNKNOWN IMPORTED)
  set_target_properties(GLIB2::GLIB2 PROPERTIES
                        IMPORTED_LOCATION "${GLIB2_LIBRARIES}"
                        INTERFACE_LINK_LIBRARIES "${GTHREAD2_LIBRARIES}"
                        INTERFACE_INCLUDE_DIRECTORIES "${GLIB2_INCLUDE_DIRS}")
endif()

mark_as_advanced(GLIB2_INCLUDE_DIRS GLIB2_INCLUDE_DIR
                 GLIB2_LIBRARIES GLIB2_LIBRARY)

include(FeatureSummary)
set_package_properties(GLIB2 PROPERTIES
  URL "https://wiki.gnome.org/Projects/GLib"
  DESCRIPTION "Event loop and utility library")

