// Copyright (C) 2013 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQNXABSTRACTCOVER_H
#define QQNXABSTRACTCOVER_H

class QQnxAbstractCover
{
public:
    virtual ~QQnxAbstractCover() {}

    virtual void updateCover() = 0;
};

#endif // QQNXABSTRACTCOVER_H
