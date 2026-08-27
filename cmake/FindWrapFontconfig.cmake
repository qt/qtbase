# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(TARGET WrapFontconfig::WrapFontconfig)
    set(WrapFontconfig_FOUND TRUE)
    return()
endif()

find_package(Fontconfig QUIET MODULE)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapFontconfig
                                  REQUIRED_VARS Fontconfig_FOUND
                                  VERSION_VAR Fontconfig_VERSION)

if(WrapFontconfig_FOUND)
    add_library(WrapFontconfig::WrapFontconfig INTERFACE IMPORTED)
    target_link_libraries(WrapFontconfig::WrapFontconfig INTERFACE Fontconfig::Fontconfig)

    # Fontconfig::Fontconfig carries only the library path. A shared libfontconfig names
    # libexpat and libfreetype in its own DT_NEEDED, an archive carries no such metadata.
    # This assumes fontconfig's default expat XML backend; one built with -Dxml-backend=libxml2
    # needs libxml2 here instead.
    get_filename_component(__fontconfig_suffix "${Fontconfig_LIBRARY}" LAST_EXT)
    if(__fontconfig_suffix STREQUAL CMAKE_STATIC_LIBRARY_SUFFIX)
        find_package(EXPAT QUIET)
        if(TARGET EXPAT::EXPAT)
            target_link_libraries(WrapFontconfig::WrapFontconfig INTERFACE EXPAT::EXPAT)
        endif()
        find_package(WrapSystemFreetype QUIET)
        if(TARGET WrapSystemFreetype::WrapSystemFreetype)
            target_link_libraries(WrapFontconfig::WrapFontconfig
                                  INTERFACE WrapSystemFreetype::WrapSystemFreetype)
        endif()
    endif()
    unset(__fontconfig_suffix)
endif()
