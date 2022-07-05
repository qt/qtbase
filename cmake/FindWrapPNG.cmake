# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

include(QtFindWrapHelper NO_POLICY_SCOPE)

qt_find_package_system_or_bundled(wrap_png
    FRIENDLY_PACKAGE_NAME "PNG"
    WRAP_PACKAGE_TARGET "WrapPNG::WrapPNG"
    WRAP_PACKAGE_FOUND_VAR_NAME "WrapPNG_FOUND"
    BUNDLED_PACKAGE_NAME "BundledLibpng"
    BUNDLED_PACKAGE_TARGET "BundledLibpng"
    SYSTEM_PACKAGE_NAME "WrapSystemPNG"
    SYSTEM_PACKAGE_TARGET "WrapSystemPNG::WrapSystemPNG"
)
