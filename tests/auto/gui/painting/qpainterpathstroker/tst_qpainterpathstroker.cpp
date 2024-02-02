// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>

#include <qfile.h>
#include <QPainterPathStroker>
#include <qmath.h>

class tst_QPainterPathStroker : public QObject
{
    Q_OBJECT

public:

private slots:
    void strokeEmptyPath();
};

void tst_QPainterPathStroker::strokeEmptyPath()
{
    QPainterPath path;
    path.moveTo(10, 10);
    path.lineTo(10, 10);
    QPainterPathStroker stroker;
    QCOMPARE(stroker.createStroke(path), path);
}

QTEST_APPLESS_MAIN(tst_QPainterPathStroker)

#include "tst_qpainterpathstroker.moc"
