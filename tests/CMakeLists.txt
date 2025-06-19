# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(QT_BUILD_STANDALONE_TESTS)
    # Add qt_find_package calls for extra dependencies that need to be found when building
    # the standalone tests here.

    # Needed for early feature values, despite it being found later on in qt_build_tests().
    # Needs to be find_package, not qt_find_package, because qt_find_package doesn't handle finding
    # component for a super-package that has already been found.
    find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS Core)

    # Modify the optimization flags specifically for qtbase standalone tests. Other projects
    # are handled by qt_enable_cmake_languages().
    qt_internal_set_up_config_optimizations_like_in_qmake()

    qt_find_package(WrapDBus1 PROVIDED_TARGETS dbus-1)
    qt_find_package(ICU 50.1 COMPONENTS i18n uc data PROVIDED_TARGETS ICU::i18n ICU::uc ICU::data)
    qt_find_package(WrapOpenSSL PROVIDED_TARGETS WrapOpenSSL::WrapOpenSSL)
    qt_find_package(WrapOpenSSLHeaders PROVIDED_TARGETS WrapOpenSSLHeaders::WrapOpenSSLHeaders)
endif()
qt_build_tests()
