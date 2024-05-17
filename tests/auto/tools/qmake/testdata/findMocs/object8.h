// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QObject>
#define Q_OBJECTOID_THING // empty

class Object8 : public QObject
{
    Q_OBJECT\
OID_THING
};

// Now we poison moc, just to make sure this doesn't get moc'ed :)
class NonMocObject8
/*  : QObject */
{
    /* qmake ignore Q_OBJECT */
    Q_OBJECT
};
