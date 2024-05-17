// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef WRAPPER_H
#define WRAPPER_H

QT_BEGIN_NAMESPACE
template <class T> class QSharedPointer;
QT_END_NAMESPACE

class Wrapper
{
public:
    QSharedPointer<int> ptr;
    Wrapper(const QSharedPointer<int> &);
    ~Wrapper();

    static Wrapper create();
};

#endif // WRAPPER_H
