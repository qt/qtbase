find_package(PkgConfig QUIET)

pkg_check_modules(XKB_COMMON_X11 "xkbcommon-x11>=0.4.1" IMPORTED_TARGET)

if (NOT TARGET PkgConfig::XKB_COMMON_X11)
    set(XKB_COMMON_X11_FOUND 0)
endif()
