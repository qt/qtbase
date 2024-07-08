# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(TARGET Qt::Concurrent)
    add_subdirectory(qmimedatabase-xml-builtin)
    add_subdirectory(qmimedatabase-xml-fdoxml)
endif()
if(TARGET Qt::Concurrent AND UNIX AND NOT APPLE AND NOT QNX)
    add_subdirectory(qmimedatabase-cache-builtin)
    add_subdirectory(qmimedatabase-cache-fdoxml)
endif()
