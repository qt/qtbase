/****************************************************************************
**
** Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
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

#include <QtTest/QtTest>

#include "qbytearray.h"
#include "qlinkedlist.h"
#include "qlist.h"
#include "qstring.h"
#include "qvarlengtharray.h"
#include "qvector.h"

#include <vector> // for reference

class tst_ContainerApiSymmetry : public QObject
{
    Q_OBJECT

private:
    template <typename Container>
    void front_back_impl() const;

private Q_SLOTS:
    void front_back_std_vector() { front_back_impl<std::vector<int>>(); }
    void front_back_QVector() { front_back_impl<QVector<int>>(); }
    void front_back_QList() { front_back_impl<QList<qintptr>>(); }
    void front_back_QLinkedList() { front_back_impl<QLinkedList<int>>(); }
    void front_back_QVarLengthArray() { front_back_impl<QVarLengthArray<int>>(); }
    void front_back_QString() { front_back_impl<QString>(); }
    void front_back_QStringRef() { front_back_impl<QStringRef>(); }
    void front_back_QStringView() { front_back_impl<QStringView>(); }
    void front_back_QLatin1String() { front_back_impl<QLatin1String>(); }
    void front_back_QByteArray() { front_back_impl<QByteArray>(); }
};

template <typename Container>
Container make(int size)
{
    Container c;
    int i = 1;
    while (size--)
        c.push_back(typename Container::value_type(i++));
    return c;
}

static QString s_string = QStringLiteral("\1\2\3\4\5\6\7");

template <> QStringRef    make(int size) { return s_string.leftRef(size); }
template <> QStringView   make(int size) { return QStringView(s_string).left(size); }
template <> QLatin1String make(int size) { return QLatin1String("\1\2\3\4\5\6\7", size); }

template <typename T> T clean(T &&t) { return std::forward<T>(t); }
inline QChar clean(QCharRef ch) { return ch; }
inline char clean(QByteRef ch) { return ch; }
inline char clean(QLatin1Char ch) { return ch.toLatin1(); }

template <typename Container>
void tst_ContainerApiSymmetry::front_back_impl() const
{
    using V = typename Container::value_type;
    auto c1 = make<Container>(1);
    QCOMPARE(clean(c1.front()), V(1));
    QCOMPARE(clean(c1.back()), V(1));
    QCOMPARE(clean(qAsConst(c1).front()), V(1));
    QCOMPARE(clean(qAsConst(c1).back()), V(1));

    auto c2 = make<Container>(2);
    QCOMPARE(clean(c2.front()), V(1));
    QCOMPARE(clean(c2.back()), V(2));
    QCOMPARE(clean(qAsConst(c2).front()), V(1));
    QCOMPARE(clean(qAsConst(c2).back()), V(2));
}

QTEST_APPLESS_MAIN(tst_ContainerApiSymmetry)
#include "tst_containerapisymmetry.moc"
