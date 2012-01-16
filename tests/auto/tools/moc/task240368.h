/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
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
