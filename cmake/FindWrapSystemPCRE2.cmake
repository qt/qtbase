if(TARGET WrapSystemPCRE2::WrapSystemPCRE2)
    set(WrapSystemPCRE2_FOUND TRUE)
    return()
endif()
set(WrapSystemPCRE2_REQUIRED_VARS __pcre2_found)

find_package(PCRE2 ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} COMPONENTS 16BIT QUIET)

# TODO: pcre2-16 is not the target name provided by the upstream Config file. It is PCRE2::16BIT.
# https://github.com/PCRE2Project/pcre2/blob/2410fbe3869cab403f02b94caa9ab37ee9f5854b/cmake/pcre2-config.cmake.in#L122
# We don't strictly need to handle that though, because the pkg-config code path below still
# finds the correct libraries.
set(__pcre2_target_name "PCRE2::pcre2-16")
if(PCRE2_FOUND AND TARGET "${__pcre2_target_name}")
  # Hunter case.
  set(__pcre2_found TRUE)
  if(PCRE2_VERSION)
      set(WrapSystemPCRE2_VERSION "${PCRE2_VERSION}")
  endif()
endif()

if(NOT __pcre2_found)
  list(PREPEND WrapSystemPCRE2_REQUIRED_VARS PCRE2_LIBRARIES PCRE2_INCLUDE_DIRS)

  find_package(PkgConfig QUIET)
  pkg_check_modules(PC_PCRE2 QUIET libpcre2-16)

  find_path(PCRE2_INCLUDE_DIRS
            NAMES pcre2.h
            HINTS ${PC_PCRE2_INCLUDEDIR})
  find_library(PCRE2_LIBRARY_RELEASE
              NAMES pcre2-16
              HINTS ${PC_PCRE2_LIBDIR})
  find_library(PCRE2_LIBRARY_DEBUG
              NAMES pcre2-16d pcre2-16
              HINTS ${PC_PCRE2_LIBDIR})
  include(SelectLibraryConfigurations)
  select_library_configurations(PCRE2)

  if(PC_PCRE2_VERSION)
      set(WrapSystemPCRE2_VERSION "${PC_PCRE2_VERSION}")
  endif()

  if (PCRE2_LIBRARIES AND PCRE2_INCLUDE_DIRS)
      set(__pcre2_found TRUE)
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapSystemPCRE2
                                  REQUIRED_VARS ${WrapSystemPCRE2_REQUIRED_VARS}
                                  VERSION_VAR WrapSystemPCRE2_VERSION)
if(WrapSystemPCRE2_FOUND)
    add_library(WrapSystemPCRE2::WrapSystemPCRE2 INTERFACE IMPORTED)
    if(TARGET "${__pcre2_target_name}")
        target_link_libraries(WrapSystemPCRE2::WrapSystemPCRE2 INTERFACE "${__pcre2_target_name}")
    else()
        target_link_libraries(WrapSystemPCRE2::WrapSystemPCRE2 INTERFACE ${PCRE2_LIBRARIES})
        target_include_directories(WrapSystemPCRE2::WrapSystemPCRE2 INTERFACE ${PCRE2_INCLUDE_DIRS})
    endif()
endif()
unset(__pcre2_target_name)
unset(__pcre2_found)
