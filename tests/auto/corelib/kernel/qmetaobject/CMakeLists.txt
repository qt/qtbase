# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(tst_qmetaobject_SOURCES
    tst_qmetaobject.cpp
    forwarddeclared.h
    forwarddeclared.cpp
)


#####################################################################
## tst_qmetaobject Test:
#####################################################################

qt_internal_add_test(tst_qmetaobject
    SOURCES
        ${tst_qmetaobject_SOURCES}
    LIBRARIES
        Qt::CorePrivate
)

qt_internal_add_test(tst_qmetaobject_compat
    SOURCES
        ${tst_qmetaobject_SOURCES}
    DEFINES
        USE_COMPAT_Q_ARG=1
    LIBRARIES
        Qt::CorePrivate
    NO_BATCH
)
