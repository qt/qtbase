# special case begin

add_library(QtDoubleConversion STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/bignum.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/bignum-dtoa.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/cached-powers.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/diy-fp.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/double-conversion.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/fast-dtoa.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/fixed-dtoa.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/strtod.cc
    )

target_include_directories(QtDoubleConversion PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/double-conversion>
    )

qt_install_3rdparty_library(QtDoubleConversion)
# special case end
