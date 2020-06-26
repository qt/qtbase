find_package(PkgConfig QUIET)

pkg_check_modules(Libsystemd libsystemd IMPORTED_TARGET)

if (NOT TARGET PkgConfig::Libsystemd)
    set(Libsystemd_FOUND 0)
endif()
