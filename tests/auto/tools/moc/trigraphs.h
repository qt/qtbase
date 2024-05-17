// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TRIGRAPHS_H
#define TRIGRAPHS_H

#include <QObject>

namespace AAA {
    struct BaseA {};
}

namespace BBB {
    class Foo : public QObject, public ::AAA::BaseA
    {
        Q_OBJECT
        Q_SIGNALS:
        // don't turn "> >" into ">>"
        void foo(QList<QList<int> >);
        void foo2(const QList<QList<int> > &);

        // don't turn "< :" into "<:"
        void bar(QList< ::AAA::BaseA*>);
        void bar2(const QList< ::AAA::BaseA*> &);
        void bar3(QList< ::AAA::BaseA const*>);
    };
}

#endif // TRIGRAPHS_H
