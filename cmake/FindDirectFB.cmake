find_package(PkgConfig QUIET)

pkg_check_modules(DirectFB directfb IMPORTED_TARGET)

if (NOT TARGET PkgConfig::DirectFB)
    set(DirectFB_FOUND 0)
endif()
