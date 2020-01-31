find_package(PkgConfig)

pkg_check_modules(Libudev libudev IMPORTED_TARGET)

if (NOT TARGET PkgConfig::Libudev)
    set(Libudev_FOUND 0)
endif()
