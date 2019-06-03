find_package(PkgConfig)

pkg_check_modules(Libsystemd systemd IMPORTED_TARGET)

if (NOT TARGET PkgConfig::Libsystemd)
    set(Libsystemd_FOUND 0)
endif()
