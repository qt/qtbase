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

#include <QtGui/QRasterWindow>
#include <QtTest/QtTest>
#include <QtGui/QPainter>

class tst_QRasterWindow : public QObject
{
    Q_OBJECT

private slots:
    void create();
    void basic();
};

void tst_QRasterWindow::create()
{
    QRasterWindow w;

    w.resize(640, 480);
    w.show();

    QTest::qWaitForWindowExposed(&w);
}

class PainterWindow : public QRasterWindow
{
public:
    void reset() { paintCount = 0; }

    void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE {
        ++paintCount;
        QPainter p(this);
        p.fillRect(QRect(0, 0, 100, 100), Qt::blue);
        p.end();
    }

    int paintCount;
};

void tst_QRasterWindow::basic()
{
    PainterWindow w;
    w.reset();
    w.resize(400, 400);
    w.show();
    QTest::qWaitForWindowExposed(&w);

    QVERIFY(w.paintCount >= 1);

    w.reset();
    w.update();
    int maxWait = 1000;
    while (w.paintCount == 0 && --maxWait > 0)
        QTest::qWait(10);

    QVERIFY(w.paintCount >= 1);
}

#include <tst_qrasterwindow.moc>

QTEST_MAIN(tst_QRasterWindow)
