# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_example(xmlstreamlint)
if(TARGET Qt6::Widgets)
    qt_internal_add_example(dombookmarks)
    qt_internal_add_example(streambookmarks)
endif()
if(TARGET Qt6::Network AND TARGET Qt6::Widgets)
    qt_internal_add_example(rsslisting)
endif()
