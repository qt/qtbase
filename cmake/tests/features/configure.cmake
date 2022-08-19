# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#### Features

# This belongs into gui, but the license check needs it here already.
qt_feature("top_a" PRIVATE
    LABEL "top_a"
    CONDITION ON
)
qt_feature("top_b" PUBLIC PRIVATE
    LABEL "top_b"
    AUTODETECT OFF
)
qt_feature_definition("top_a" "top_defa")

qt_feature("top_enabled" PRIVATE
    LABEL "top_enabled"
    ENABLE ON
)

qt_feature("top_disabled" PRIVATE
    LABEL "top_enabled"
    DISABLE ON
)

qt_feature("top_disabled_enabled" PRIVATE
    LABEL "top_enabled_enabled"
    DISABLE ON
    ENABLE ON
)

qt_feature("top_not_emitted" PRIVATE
    LABEL "top_not_emitted"
    EMIT_IF OFF
)

qt_extra_definition("top_extra" "PUBLIC_FOO" PUBLIC)
