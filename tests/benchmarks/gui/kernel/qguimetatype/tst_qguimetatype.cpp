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

#include <qtest.h>
#include <QtCore/qmetatype.h>
#include <QScopeGuard>

class tst_QGuiMetaType : public QObject
{
    Q_OBJECT

private slots:
    void constructInPlace_data();
    void constructInPlace();
    void constructInPlaceCopy_data();
    void constructInPlaceCopy();
private:
    void constructableGuiTypes();
};


void tst_QGuiMetaType::constructableGuiTypes()
{
    QTest::addColumn<int>("typeId");
    for (int i = QMetaType::FirstGuiType; i <= QMetaType::LastGuiType; ++i) {
        if (QMetaType metaType(i); metaType.isValid())
            QTest::newRow(metaType.name()) << i;
    }
}


void tst_QGuiMetaType::constructInPlace_data()
{
    constructableGuiTypes();
}

void tst_QGuiMetaType::constructInPlace()
{
    QFETCH(int, typeId);
    QMetaType type(typeId);
    int size = type.sizeOf();
    void *storage = qMallocAligned(size, 2 * sizeof(qlonglong));
    auto cleanUp = qScopeGuard([&]() {
        qFreeAligned(storage);
    });
    QCOMPARE(type.construct(storage, /*copy=*/0), storage);
    type.destruct(storage);
    QBENCHMARK {
        for (int i = 0; i < 100000; ++i) {
            type.construct(storage, /*copy=*/0);
            type.destruct(storage);
        }
    }
}

void tst_QGuiMetaType::constructInPlaceCopy_data()
{
    constructableGuiTypes();
}

void tst_QGuiMetaType::constructInPlaceCopy()
{
    QFETCH(int, typeId);
    QMetaType type(typeId);
    int size = type.sizeOf();
    void *storage = qMallocAligned(size, 2 * sizeof(qlonglong));
    void *other = type.create();
    auto cleanUp = qScopeGuard([&]() {
        type.destroy(other);
        qFreeAligned(storage);
    });
    QCOMPARE(type.construct(storage, other), storage);
    type.destruct(storage);
    QBENCHMARK {
        for (int i = 0; i < 100000; ++i) {
            type.construct(storage, other);
            type.destruct(storage);
        }
    }
}

QTEST_MAIN(tst_QGuiMetaType)
#include "tst_qguimetatype.moc"
