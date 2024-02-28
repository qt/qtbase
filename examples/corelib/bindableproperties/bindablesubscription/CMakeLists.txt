# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

qt_add_executable(bindablesubscription
    main.cpp
    bindablesubscription.cpp
    bindablesubscription.h
    bindableuser.cpp
    bindableuser.h
)

target_link_libraries(bindablesubscription PRIVATE
    bindableproperties_shared
)

