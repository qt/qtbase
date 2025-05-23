// Copyright (C) 2013 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QQNXABSTRACTCOVER_H
#define QQNXABSTRACTCOVER_H

class QQnxAbstractCover
{
public:
    virtual ~QQnxAbstractCover() {}

    virtual void updateCover() = 0;
};

#endif // QQNXABSTRACTCOVER_H
