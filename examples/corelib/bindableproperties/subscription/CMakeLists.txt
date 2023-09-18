# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_add_executable(subscription
    main.cpp
    subscription.cpp subscription.h
    user.cpp user.h
)

target_link_libraries(subscription PRIVATE
    bindableproperties_shared
)
