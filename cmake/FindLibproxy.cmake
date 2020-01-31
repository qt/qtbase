find_package(PkgConfig)

pkg_check_modules(Libproxy libproxy-1.0 IMPORTED_TARGET)

if (NOT TARGET PkgConfig::Libproxy)
    set(Libproxy_FOUND 0)
endif()
