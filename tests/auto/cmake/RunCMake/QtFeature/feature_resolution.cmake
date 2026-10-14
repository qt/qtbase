# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_library(FeatureTest INTERFACE)
qt_feature_module_begin(LIBRARY FeatureTest NO_HEADERS)

qt_feature("condition_on" PRIVATE
    LABEL "condition_on"
    CONDITION ON
)
qt_feature("condition_off" PRIVATE
    LABEL "condition_off"
    CONDITION OFF
)
qt_feature("autodetect_off" PUBLIC PRIVATE
    LABEL "autodetect_off"
    AUTODETECT OFF
)
qt_feature("enabled" PRIVATE
    LABEL "enabled"
    ENABLE ON
)
qt_feature("disabled" PRIVATE
    LABEL "disabled"
    DISABLE ON
)
# DISABLE wins over ENABLE.
qt_feature("disabled_enabled" PRIVATE
    LABEL "disabled_enabled"
    DISABLE ON
    ENABLE ON
)
# A feature that is not emitted is OFF even though its condition holds.
qt_feature("not_emitted" PRIVATE
    LABEL "not_emitted"
    EMIT_IF OFF
)

qt_feature_module_end(FeatureTest)

set(expected_on condition_on enabled)
set(expected_off condition_off autodetect_off disabled disabled_enabled not_emitted)

include("${CMAKE_CURRENT_LIST_DIR}/check_features.cmake")

add_subdirectory(subdir)
