# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(desktopsettingsaware)
add_subdirectory(modal)
add_subdirectory(test)

add_dependencies(tst_qapplication
    desktopsettingsaware_helper
    modal_helper
)
