# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

function(qt_internal_disable_find_package_global_promotion target)
    set_target_properties("${target}" PROPERTIES _qt_no_promote_global TRUE)
endfunction()
