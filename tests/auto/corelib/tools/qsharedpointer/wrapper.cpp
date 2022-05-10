// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef QT_SHAREDPOINTER_TRACK_POINTERS
# undef QT_SHAREDPOINTER_TRACK_POINTERS
#endif
#include <QtCore/qsharedpointer.h>
#include "wrapper.h"

Wrapper::Wrapper(const QSharedPointer<int> &value)
        : ptr(value)
{
}

Wrapper::~Wrapper()
{
}

Wrapper Wrapper::create()
{
    return Wrapper(QSharedPointer<int>(new int(-47)));
}
