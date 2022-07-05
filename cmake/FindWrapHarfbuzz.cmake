# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

include(QtFindWrapHelper NO_POLICY_SCOPE)

qt_find_package_system_or_bundled(wrap_harfbuzz
    FRIENDLY_PACKAGE_NAME "Harfbuzz"
    WRAP_PACKAGE_TARGET "WrapHarfbuzz::WrapHarfbuzz"
    WRAP_PACKAGE_FOUND_VAR_NAME "WrapHarfbuzz_FOUND"
    BUNDLED_PACKAGE_NAME "BundledHarfbuzz"
    BUNDLED_PACKAGE_TARGET "BundledHarfbuzz"
    SYSTEM_PACKAGE_NAME "WrapSystemHarfbuzz"
    SYSTEM_PACKAGE_TARGET "WrapSystemHarfbuzz::WrapSystemHarfbuzz"
)
