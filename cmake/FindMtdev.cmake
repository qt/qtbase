find_package(PkgConfig QUIET)

pkg_check_modules(Mtdev mtdev IMPORTED_TARGET)

if (NOT TARGET PkgConfig::Mtdev)
    set(Mtdev_FOUND 0)
endif()
