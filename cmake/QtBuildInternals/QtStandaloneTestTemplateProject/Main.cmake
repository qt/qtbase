# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Includes QtSetup and friends for private CMake API.
qt_internal_project_setup()
qt_build_internals_set_up_private_api()

# Find all StandaloneTestsConfig.cmake files, and include them
# This will find all Qt packages that are required for standalone tests.
# It will find more packages that needed for a certain test, but will ensure any test can
# be built.
qt_get_standalone_tests_config_files_path(standalone_tests_config_path)

file(GLOB config_files "${standalone_tests_config_path}/*")
foreach(file ${config_files})
    include("${file}")
endforeach()

# Set language standards after finding Core, because that's when the relevant
# feature variables are available.
qt_set_language_standards()

# Just before adding the test, change the local (non-cache) install prefix to something other than
# the Qt install prefix, so that tests don't try to install and pollute the Qt install prefix.
# Needs to be called after qt_get_standalone_tests_confg_files_path().
qt_set_up_fake_standalone_tests_install_prefix()
