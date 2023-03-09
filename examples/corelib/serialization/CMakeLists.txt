# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_example(cbordump)
qt_internal_add_example(convert)
qt_internal_add_example(savegame)
if(TARGET Qt6::Network AND TARGET Qt6::Widgets)
    qt_internal_add_example(rsslisting)
endif()
if(TARGET Qt6::Widgets)
    qt_internal_add_example(streambookmarks)
endif()
