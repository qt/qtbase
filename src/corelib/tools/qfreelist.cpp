// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qfreelist_p.h"

QT_BEGIN_NAMESPACE

// default sizes and offsets (no need to define these when customizing)
namespace QFreeListDefaultConstantsPrivate {
enum {
    Offset0 = 0x00000000,
    Offset1 = 0x00008000,
    Offset2 = 0x00080000,
    Offset3 = 0x00800000,

    Size0 = Offset1 - Offset0,
    Size1 = Offset2 - Offset1,
    Size2 = Offset3 - Offset2,
    Size3 = QFreeListDefaultConstants::MaxIndex - Offset3
};
}

Q_CONSTINIT const int QFreeListDefaultConstants::Sizes[QFreeListDefaultConstants::BlockCount] = {
    QFreeListDefaultConstantsPrivate::Size0,
    QFreeListDefaultConstantsPrivate::Size1,
    QFreeListDefaultConstantsPrivate::Size2,
    QFreeListDefaultConstantsPrivate::Size3
};

QT_END_NAMESPACE

