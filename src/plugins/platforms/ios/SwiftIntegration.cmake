# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(CMAKE_Swift_COMPILER_TARGET arm64-apple-xros)
if($CACHE{CMAKE_OSX_SYSROOT} MATCHES "^[a-z]+simulator$")
    set(CMAKE_Swift_COMPILER_TARGET "${CMAKE_Swift_COMPILER_TARGET}-simulator")
endif()

cmake_policy(SET CMP0157 NEW)
enable_language(Swift)

# Verify that we have a new enough compiler
if("${CMAKE_Swift_COMPILER_VERSION}" VERSION_LESS 5.9)
  message(FATAL_ERROR "Swift 5.9 required for C++ interoperability")
endif()

get_target_property(QT_CORE_INCLUDES Qt6::Core INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(QT_GUI_INCLUDES Qt6::Gui INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(QT_CORE_PRIVATE_INCLUDES Qt6::CorePrivate INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(QT_GUI_PRIVATE_INCLUDES Qt6::GuiPrivate INTERFACE_INCLUDE_DIRECTORIES)

set(target QIOSIntegrationPluginSwift)
# Swift library
set(SWIFT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/qiosapplication.swift"
)
add_library(${target} STATIC ${SWIFT_SOURCES})
set_target_properties(${target} PROPERTIES
    Swift_MODULE_NAME ${target})
target_include_directories(${target} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${QT_CORE_INCLUDES}"
    "${QT_GUI_INCLUDES}"
    "${QT_CORE_PRIVATE_INCLUDES}"
    "${QT_GUI_PRIVATE_INCLUDES}"

)
target_compile_options(${target} PRIVATE
  $<$<COMPILE_LANGUAGE:Swift>:-cxx-interoperability-mode=default>
  $<$<COMPILE_LANGUAGE:Swift>:-Xcc -std=c++17>)

# Swift to C++ bridging header
set(SWIFT_BRIDGING_HEADER "${CMAKE_CURRENT_BINARY_DIR}/qiosswiftintegration.h")
list(TRANSFORM QT_CORE_INCLUDES PREPEND "-I")
list(TRANSFORM QT_GUI_INCLUDES PREPEND "-I")
list(TRANSFORM QT_CORE_PRIVATE_INCLUDES PREPEND "-I")
list(TRANSFORM QT_GUI_PRIVATE_INCLUDES PREPEND "-I")
add_custom_command(
    COMMAND
      ${CMAKE_Swift_COMPILER} -frontend -typecheck
      ${SWIFT_SOURCES}
      -I ${CMAKE_CURRENT_SOURCE_DIR}
      ${QT_CORE_INCLUDES}
      ${QT_GUI_INCLUDES}
      ${QT_CORE_PRIVATE_INCLUDES}
      ${QT_GUI_PRIVATE_INCLUDES}
      -sdk ${CMAKE_OSX_SYSROOT}
      -module-name ${target}
      -cxx-interoperability-mode=default
      -Xcc -std=c++17
      -emit-clang-header-path "${SWIFT_BRIDGING_HEADER}"
      -target ${CMAKE_Swift_COMPILER_TARGET}
    OUTPUT
       "${SWIFT_BRIDGING_HEADER}"
    DEPENDS
        ${SWIFT_SOURCES}
  )

set(header_target "${target}Header")
add_custom_target(${header_target}
    DEPENDS "${SWIFT_BRIDGING_HEADER}"
)
# Make sure the "'__bridge_transfer' casts have no effect when not using ARC"
# warning doesn't break warnings-are-error builds.
target_compile_options(${target} INTERFACE
  -Wno-error=arc-bridge-casts-disallowed-in-nonarc)

add_dependencies(${target} ${header_target})
