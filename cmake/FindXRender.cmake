include(FindPkgConfig)

if(NOT TARGET PkgConfig::xrender)
    pkg_check_modules(XRender xrender IMPORTED_TARGET)
endif()
