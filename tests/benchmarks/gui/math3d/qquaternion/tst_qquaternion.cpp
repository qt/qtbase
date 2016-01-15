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
#include <QQuaternion>

class tst_QQuaternion : public QObject
{
    Q_OBJECT

public:
    tst_QQuaternion();
    virtual ~tst_QQuaternion();

public slots:
    void init();
    void cleanup();

private slots:
    void multiply_data();
    void multiply();
};

tst_QQuaternion::tst_QQuaternion()
{
}

tst_QQuaternion::~tst_QQuaternion()
{
}

void tst_QQuaternion::init()
{
}

void tst_QQuaternion::cleanup()
{
}

void tst_QQuaternion::multiply_data()
{
    QTest::addColumn<float>("x1");
    QTest::addColumn<float>("y1");
    QTest::addColumn<float>("z1");
    QTest::addColumn<float>("w1");
    QTest::addColumn<float>("x2");
    QTest::addColumn<float>("y2");
    QTest::addColumn<float>("z2");
    QTest::addColumn<float>("w2");

    QTest::newRow("null")
        << 0.0f << 0.0f << 0.0f << 0.0f
        << 0.0f << 0.0f << 0.0f << 0.0f;

    QTest::newRow("unitvec")
        << 1.0f << 0.0f << 0.0f << 1.0f
        << 0.0f << 1.0f << 0.0f << 1.0f;

    QTest::newRow("complex")
        << 1.0f << 2.0f << 3.0f << 7.0f
        << 4.0f << 5.0f << 6.0f << 8.0f;
}

void tst_QQuaternion::multiply()
{
    QFETCH(float, x1);
    QFETCH(float, y1);
    QFETCH(float, z1);
    QFETCH(float, w1);
    QFETCH(float, x2);
    QFETCH(float, y2);
    QFETCH(float, z2);
    QFETCH(float, w2);

    QQuaternion q1(w1, x1, y1, z1);
    QQuaternion q2(w2, x2, y2, z2);

    QBENCHMARK {
        QQuaternion q3 = q1 * q2;
    }
}

QTEST_MAIN(tst_QQuaternion)
#include "tst_qquaternion.moc"
