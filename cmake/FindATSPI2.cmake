include(FindPkgConfig)

if(NOT TARGET PkgConfig::ATSPI2)
    pkg_check_modules(ATSPI2 atspi-2 IMPORTED_TARGET)
endif()
