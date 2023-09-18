# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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

