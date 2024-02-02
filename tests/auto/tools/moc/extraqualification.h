// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef EXTRAQUALIFICATION_H
#define EXTRAQUALIFICATION_H

#include <QObject>

class Test : public QObject
{
    Q_OBJECT
public slots:
    // this is invalid code that does not compile, the extra qualification
    // is bad. However for example older gccs silently accept it, so customers
    // can write the code and moc generates bad metadata. So instead moc should
    // now write out a warning and /not/ generate any code, because the code is
    // bad and with a decent compiler it won't compile anyway.
    void Test::badFunctionDeclaration() {}

public:
    Q_SLOT void Test::anotherOne() {}
};
#endif // EXTRAQUALIFICATION_H
