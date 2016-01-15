/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
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
