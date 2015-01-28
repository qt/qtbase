/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include <qpolygon.h>
#include <qpainterpath.h>
#include <math.h>

#include <qpainter.h>

class tst_QPolygon : public QObject
{
    Q_OBJECT

public:
    tst_QPolygon();

private slots:
    void makeEllipse();
    void swap();
};

tst_QPolygon::tst_QPolygon()
{
}

void tst_QPolygon::makeEllipse()
{
    // create an ellipse with R1 = R2 = R, i.e. a circle
    QPolygon pa;
    const int R = 50; // radius
    QPainterPath path;
    path.addEllipse(0, 0, 2*R, 2*R);
    pa = path.toSubpathPolygons().at(0).toPolygon();

    int i;
    // make sure that all points are R+-1 away from the center
    bool err = false;
    for (i = 1; i < pa.size(); i++) {
        QPoint p = pa.at(i);
        double r = sqrt(pow(double(p.x() - R), 2.0) + pow(double(p.y() - R), 2.0));
        // ### too strict ? at least from visual inspection it looks
        // quite odd around the main axes. 2.0 passes easily.
        err |= (qAbs(r - double(R)) > 2.0);
    }
    QVERIFY( !err );
}

void tst_QPolygon::swap()
{
    QPolygon p1(QVector<QPoint>() << QPoint(0,0) << QPoint(10,10) << QPoint(-10,10));
    QPolygon p2(QVector<QPoint>() << QPoint(0,0) << QPoint( 0,10) << QPoint( 10,10) << QPoint(10,0));
    p1.swap(p2);
    QCOMPARE(p1.count(),4);
    QCOMPARE(p2.count(),3);
}

QTEST_APPLESS_MAIN(tst_QPolygon)
#include "tst_qpolygon.moc"
