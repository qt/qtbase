/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <qmath.h>

static const qreal PI = 3.14159265358979323846264338327950288;

class tst_QMath : public QObject
{
    Q_OBJECT
private slots:
    void fastSinCos();
};

void tst_QMath::fastSinCos()
{
    // Test evenly spaced angles from 0 to 2pi radians.
    const int LOOP_COUNT = 100000;
    for (int i = 0; i < LOOP_COUNT; ++i) {
        qreal angle = i * 2 * PI / (LOOP_COUNT - 1);
        QVERIFY(qAbs(qSin(angle) - qFastSin(angle)) < 1e-5);
        QVERIFY(qAbs(qCos(angle) - qFastCos(angle)) < 1e-5);
    }
}

QTEST_APPLESS_MAIN(tst_QMath)

#include "tst_qmath.moc"
