# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(qt_internal_disable_find_package_global_promotion target)
    set_target_properties("${target}" PROPERTIES _qt_no_promote_global TRUE)
endfunction()
