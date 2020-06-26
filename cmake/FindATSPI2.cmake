find_package(PkgConfig QUIET)

pkg_check_modules(ATSPI2 atspi-2 IMPORTED_TARGET)

if (NOT TARGET PkgConfig::ATSPI2)
    set(ATSPI2_FOUND 0)
endif()
