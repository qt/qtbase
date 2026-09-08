# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(test_standalone_tools_packages)

# A Tools package must be usable without finding the Qt6 package first.
find_package(Qt6CoreTools REQUIRED)

if(NOT TARGET Qt6::moc)
    message(FATAL_ERROR "Qt6::moc target was not created")
endif()

# Tools packages are host packages. Pulling in the Qt6 package would pull in the host Qt for a
# project that cross-builds.
if(Qt6_FOUND)
    message(FATAL_ERROR "find_package(Qt6CoreTools) must not find the Qt6 package")
endif()

# Dependencies on other Tools packages must be resolved as well.
if(TEST_GUI_TOOLS)
    find_package(Qt6GuiTools REQUIRED)
endif()

# Finding the Qt6 package afterwards must still work.
find_package(Qt6 REQUIRED COMPONENTS Core)

if(NOT TARGET Qt6::Core)
    message(FATAL_ERROR "Qt6::Core target was not created")
endif()
