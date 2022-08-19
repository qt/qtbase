# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(asyncify_exec)
add_subdirectory(eventloop_auto)
add_subdirectory(main_exec)
add_subdirectory(main_noexec)
add_subdirectory(thread_exec)
if(QT_FEATURE_widgets)
add_subdirectory(dialog_exec)
endif()
