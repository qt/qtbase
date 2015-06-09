/****************************************************************************
**
** Copyright (C) 2015 Olivier Goffart <ogoffart@woboq.com>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <QtCore/qobject.h>
#include <QtCore/qmetaobject.h>

class tst_QMetaEnum : public QObject
{
    Q_OBJECT
public:
    enum SuperEnum { SuperValue1 = 1 , SuperValue2 = 2 };
    Q_ENUM(SuperEnum)

private slots:
    void fromType();
    void valuesToKeys_data();
    void valuesToKeys();
};

void tst_QMetaEnum::fromType()
{
    QMetaEnum meta = QMetaEnum::fromType<SuperEnum>();
    QVERIFY(meta.isValid());
    QCOMPARE(meta.name(), "SuperEnum");
    QCOMPARE(meta.enclosingMetaObject(), &staticMetaObject);
    QCOMPARE(meta.keyCount(), 2);
}

Q_DECLARE_METATYPE(Qt::WindowFlags)

void tst_QMetaEnum::valuesToKeys_data()
{
   QTest::addColumn<Qt::WindowFlags>("windowFlags");
   QTest::addColumn<QByteArray>("expected");

   QTest::newRow("Window")
       << Qt::WindowFlags(Qt::Window)
       << QByteArrayLiteral("Window");

   // Verify that Qt::Dialog does not cause 'Window' to appear in the output.
   QTest::newRow("Frameless_Dialog")
       << (Qt::Dialog | Qt::FramelessWindowHint)
       << QByteArrayLiteral("Dialog|FramelessWindowHint");

   // Similarly, Qt::WindowMinMaxButtonsHint should not show up as
   // WindowMinimizeButtonHint|WindowMaximizeButtonHint
   QTest::newRow("Tool_MinMax_StaysOnTop")
       << (Qt::Tool | Qt::WindowMinMaxButtonsHint | Qt::WindowStaysOnTopHint)
       << QByteArrayLiteral("Tool|WindowMinMaxButtonsHint|WindowStaysOnTopHint");
}

void tst_QMetaEnum::valuesToKeys()
{
    QFETCH(Qt::WindowFlags, windowFlags);
    QFETCH(QByteArray, expected);

    QMetaEnum me = QMetaEnum::fromType<Qt::WindowFlags>();
    QCOMPARE(me.valueToKeys(windowFlags), expected);
}

Q_STATIC_ASSERT(QtPrivate::IsQEnumHelper<tst_QMetaEnum::SuperEnum>::Value);
Q_STATIC_ASSERT(QtPrivate::IsQEnumHelper<Qt::WindowFlags>::Value);
Q_STATIC_ASSERT(QtPrivate::IsQEnumHelper<Qt::Orientation>::Value);
Q_STATIC_ASSERT(!QtPrivate::IsQEnumHelper<int>::Value);
Q_STATIC_ASSERT(!QtPrivate::IsQEnumHelper<QObject>::Value);
Q_STATIC_ASSERT(!QtPrivate::IsQEnumHelper<QObject*>::Value);
Q_STATIC_ASSERT(!QtPrivate::IsQEnumHelper<void>::Value);

QTEST_MAIN(tst_QMetaEnum)
#include "tst_qmetaenum.moc"
