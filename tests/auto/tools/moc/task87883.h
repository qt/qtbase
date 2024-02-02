// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TASK87883_H
#define TASK87883_H
/*
    The bug is triggered only if there is a multiline comment after an #include
    statement that in the following line contains a single quote but lacks a finishing
    quote. So Moc tries to find the end quote, doesn't find it and by the skips over the
    everything, including important class declarations :)
 */

#include <qobject.h> /* blah
foo ' bar */

class Task87883 : public QObject
{
    Q_OBJECT
public:
    inline Task87883() {}
};

#endif // TASK87883_H
