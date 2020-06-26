find_package(PkgConfig QUIET)

if(NOT TARGET PkgConfig::XRender)
    pkg_check_modules(XRender xrender IMPORTED_TARGET)

    if (NOT TARGET PkgConfig::XRender)
        set(XRender_FOUND 0)
    endif()
else()
    set(XRender_FOUND 1)
endif()
