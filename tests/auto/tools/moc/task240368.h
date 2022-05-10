// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// moc parsing issue with "unsigned" subphrase

#ifndef TASK240368_H
#define TASK240368_H

#include <QObject>

typedef struct { unsigned _1; } unsigned1;
typedef struct { unsigned qi; } unsignedQImage;

class TypenameWithUnsigned : public QObject {

    Q_OBJECT

public slots:

    void a(unsigned) { }
    void b(unsigned u) { Q_UNUSED(u); }
    void c(unsigned*) { }
    void d(unsigned* p) { Q_UNUSED(p); }
    void e(unsigned&) { }
    void f(unsigned& r) { Q_UNUSED(r); }
    void g(unsigned1) { }
    void h(unsigned1 u1) { Q_UNUSED(u1); }
    void i(unsigned,unsigned1) { }
    void j(unsigned1,unsigned) { }
    void k(unsignedQImage) { }
    void l(unsignedQImage uqi) { Q_UNUSED(uqi); }

};

#endif
