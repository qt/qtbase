find_package(PkgConfig QUIET)

pkg_check_modules(GTK3 "gtk+-3.0 >= 3.6" IMPORTED_TARGET)

if (NOT TARGET PkgConfig::GTK3)
    set(GTK3_FOUND 0)
endif()
