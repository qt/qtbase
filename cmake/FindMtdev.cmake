find_package(PkgConfig)

pkg_check_modules(Mtdev mtdev IMPORTED_TARGET)

if (NOT TARGET PkgConfig::MtDev)
    set(Mtdev_FOUND 0)
endif()
