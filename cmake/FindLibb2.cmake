find_package(PkgConfig QUIET)

pkg_check_modules(Libb2 libb2 IMPORTED_TARGET)

if (NOT TARGET PkgConfig::Libb2)
    set(Libb2_FOUND 0)
endif()
