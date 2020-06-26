if(TARGET WrapSystemPCRE2::WrapSystemPCRE2)
    set(WrapSystemPCRE2_FOUND TRUE)
    return()
endif()

find_package(PCRE2 CONFIG QUIET)

if(PCRE2_FOUND AND TARGET PCRE2::pcre2-16)
  # Hunter case.
  add_library(WrapSystemPCRE2::WrapSystemPCRE2 INTERFACE IMPORTED)
  target_link_libraries(WrapSystemPCRE2::WrapSystemPCRE2 INTERFACE PCRE2::pcre2-16)
  set(WrapSystemPCRE2_FOUND TRUE)
else()
  find_library(PCRE2_LIBRARIES NAMES pcre2-16)
  find_path(PCRE2_INCLUDE_DIRS pcre2.h)

  if (PCRE2_LIBRARIES AND PCRE2_INCLUDE_DIRS)
      add_library(WrapSystemPCRE2::WrapSystemPCRE2 INTERFACE IMPORTED)
      target_link_libraries(WrapSystemPCRE2::WrapSystemPCRE2 INTERFACE ${PCRE2_LIBRARIES})
      target_include_directories(WrapSystemPCRE2::WrapSystemPCRE2 INTERFACE ${PCRE2_INCLUDE_DIRS})
      set(WrapSystemPCRE2_FOUND TRUE)
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapSystemPCRE2 DEFAULT_MSG WrapSystemPCRE2_FOUND)
