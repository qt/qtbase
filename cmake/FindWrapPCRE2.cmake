if(TARGET WrapPCRE2::WrapPCRE2)
    set(WrapPCRE2_FOUND TRUE)
    return()
endif()

find_package(PCRE2 CONFIG QUIET)

if(PCRE2_FOUND AND TARGET PCRE2::pcre2-16)
  # Hunter case.
  add_library(WrapPCRE2::WrapPCRE2 INTERFACE IMPORTED)
  target_link_libraries(WrapPCRE2::WrapPCRE2 INTERFACE PCRE2::pcre2-16)
  set(WrapPCRE2_FOUND TRUE)
else()
  find_library(PCRE2_LIBRARIES NAMES pcre2-16)
  find_path(PCRE2_INCLUDE_DIRS pcre2.h)

  if (PCRE2_LIBRARIES AND PCRE2_INCLUDE_DIRS)
      add_library(WrapPCRE2::WrapPCRE2 INTERFACE IMPORTED)
      target_link_libraries(WrapPCRE2::WrapPCRE2 INTERFACE ${PCRE2_LIBRARIES})
      target_include_directories(WrapPCRE2::WrapPCRE2 INTERFACE ${PCRE2_INCLUDE_DIRS})
      set(WrapPCRE2_FOUND TRUE)
  endif()
endif()
