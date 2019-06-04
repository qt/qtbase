include(FindPkgConfig)

if(NOT TARGET PkgConfig::XRender)
    pkg_check_modules(XRender xrender IMPORTED_TARGET)

    if (NOT TARGET PkgConfig::XRender)
        set(XRender_FOUND 0)
    endif()
endif()
