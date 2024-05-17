// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QObject>

/*/ <-- Inside a comment

class Object4 : public QObject
{
    Q_OBJECT
};

Comment ends there --> /*/

// Now we poison moc, just to make sure this doesn't get moc'ed :)
class NonMocObject4
/*  : QObject */
{
    /* qmake ignore Q_OBJECT */
    Q_OBJECT
};
