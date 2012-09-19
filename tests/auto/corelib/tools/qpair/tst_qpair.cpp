/****************************************************************************
**
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#include <QPair>

class tst_QPair : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void dummy() {}
};

class C { char _[4]; };
class M { char _[4]; };
class P { char _[4]; };

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(M, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(P, Q_PRIMITIVE_TYPE);
QT_END_NAMESPACE

// avoid the comma:
typedef QPair<C,C> QPairCC;
typedef QPair<C,M> QPairCM;
typedef QPair<C,P> QPairCP;
typedef QPair<M,C> QPairMC;
typedef QPair<M,M> QPairMM;
typedef QPair<M,P> QPairMP;
typedef QPair<P,C> QPairPC;
typedef QPair<P,M> QPairPM;
typedef QPair<P,P> QPairPP;

Q_STATIC_ASSERT( QTypeInfo<QPairCC>::isComplex);
Q_STATIC_ASSERT( QTypeInfo<QPairCC>::isStatic );

Q_STATIC_ASSERT( QTypeInfo<QPairCM>::isComplex);
Q_STATIC_ASSERT( QTypeInfo<QPairCM>::isStatic );

Q_STATIC_ASSERT( QTypeInfo<QPairCP>::isComplex);
Q_STATIC_ASSERT( QTypeInfo<QPairCP>::isStatic );

Q_STATIC_ASSERT( QTypeInfo<QPairMC>::isComplex);
Q_STATIC_ASSERT( QTypeInfo<QPairMC>::isStatic );

Q_STATIC_ASSERT( QTypeInfo<QPairMM>::isComplex);
Q_STATIC_ASSERT(!QTypeInfo<QPairMM>::isStatic );

Q_STATIC_ASSERT( QTypeInfo<QPairMP>::isComplex);
Q_STATIC_ASSERT(!QTypeInfo<QPairMP>::isStatic );

Q_STATIC_ASSERT( QTypeInfo<QPairPC>::isComplex);
Q_STATIC_ASSERT( QTypeInfo<QPairPC>::isStatic );

Q_STATIC_ASSERT( QTypeInfo<QPairPM>::isComplex);
Q_STATIC_ASSERT(!QTypeInfo<QPairPM>::isStatic );

Q_STATIC_ASSERT(!QTypeInfo<QPairPP>::isComplex);
Q_STATIC_ASSERT(!QTypeInfo<QPairPP>::isStatic );

Q_STATIC_ASSERT(!QTypeInfo<QPairPP>::isDummy  );
Q_STATIC_ASSERT(!QTypeInfo<QPairPP>::isPointer);


QTEST_APPLESS_MAIN(tst_QPair)
#include "tst_qpair.moc"
