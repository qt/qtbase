/****************************************************************************
**
** Copyright (C) 2014 John Layt <jlayt@kde.org>
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
#include <QtGui/qpagelayout.h>

class tst_QPageLayout : public QObject
{
    Q_OBJECT

private slots:
    void invalid();
    void basics();
    void setGetMargins();
};

void tst_QPageLayout::invalid()
{
    // Invalid
    QPageLayout invalid = QPageLayout();
    QCOMPARE(invalid.isValid(), false);
    invalid = QPageLayout(QPageSize(), QPageLayout::Portrait, QMarginsF());
    QCOMPARE(invalid.isValid(), false);
}

void tst_QPageLayout::basics()
{
    // Simple A4, no margins
    QPageLayout simple = QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait, QMarginsF(0, 0, 0, 0));
    QCOMPARE(simple.isValid(), true);
    QCOMPARE(simple.pageSize().id(), QPageSize::A4);
    QCOMPARE(simple.orientation(), QPageLayout::Portrait);
    QCOMPARE(simple.margins(), QMarginsF(0, 0, 0, 0));
    QCOMPARE(simple.margins(QPageLayout::Millimeter), QMarginsF(0, 0, 0, 0));
    QCOMPARE(simple.marginsPoints(), QMargins(0, 0, 0, 0));
    QCOMPARE(simple.marginsPixels(72), QMargins(0, 0, 0, 0));
    QCOMPARE(simple.minimumMargins(), QMarginsF(0, 0, 0, 0));
    QCOMPARE(simple.maximumMargins(), QMarginsF(595, 842, 595, 842));
    QCOMPARE(simple.fullRect(), QRectF(0, 0, 595, 842));
    QCOMPARE(simple.fullRect(QPageLayout::Millimeter), QRectF(0, 0, 210, 297));
    QCOMPARE(simple.fullRectPoints(), QRect(0, 0, 595, 842));
    QCOMPARE(simple.fullRectPixels(72), QRect(0, 0, 595, 842));
    QCOMPARE(simple.paintRect(), QRectF(0, 0, 595, 842));
    QCOMPARE(simple.paintRect(QPageLayout::Millimeter), QRectF(0, 0, 210, 297));
    QCOMPARE(simple.paintRectPoints(), QRect(0, 0, 595, 842));
    QCOMPARE(simple.paintRectPixels(72), QRect(0, 0, 595, 842));

    const QPageLayout a4portrait = simple;
    QVERIFY(a4portrait == simple);

    // Change orientation
    simple.setOrientation(QPageLayout::Landscape);
    QVERIFY(simple != a4portrait);
    QCOMPARE(simple.orientation(), QPageLayout::Landscape);
    QCOMPARE(simple.margins(), QMarginsF(0, 0, 0, 0));
    QCOMPARE(simple.minimumMargins(), QMarginsF(0, 0, 0, 0));
    QCOMPARE(simple.maximumMargins(), QMarginsF(842, 595, 842, 595));
    QCOMPARE(simple.fullRect(), QRectF(0, 0, 842, 595));
    QCOMPARE(simple.fullRect(QPageLayout::Millimeter), QRectF(0, 0, 297, 210));
    QCOMPARE(simple.fullRectPoints(), QRect(0, 0, 842, 595));
    QCOMPARE(simple.fullRectPixels(72), QRect(0, 0, 842, 595));
    QCOMPARE(simple.paintRect(), QRectF(0, 0, 842, 595));
    QCOMPARE(simple.paintRect(QPageLayout::Millimeter), QRectF(0, 0, 297, 210));
    QCOMPARE(simple.paintRectPoints(), QRect(0, 0, 842, 595));
    QCOMPARE(simple.paintRectPixels(72), QRect(0, 0, 842, 595));

    // Change mode
    QCOMPARE(simple.mode(), QPageLayout::StandardMode);
    simple.setMode(QPageLayout::FullPageMode);
    QCOMPARE(simple.mode(), QPageLayout::FullPageMode);
    QCOMPARE(simple.orientation(), QPageLayout::Landscape);
    QCOMPARE(simple.margins(), QMarginsF(0, 0, 0, 0));
    QCOMPARE(simple.minimumMargins(), QMarginsF(0, 0, 0, 0));
    QCOMPARE(simple.maximumMargins(), QMarginsF(842, 595, 842, 595));
    QCOMPARE(simple.fullRect(), QRectF(0, 0, 842, 595));
    QCOMPARE(simple.fullRect(QPageLayout::Millimeter), QRectF(0, 0, 297, 210));
    QCOMPARE(simple.fullRectPoints(), QRect(0, 0, 842, 595));
    QCOMPARE(simple.fullRectPixels(72), QRect(0, 0, 842, 595));
    QCOMPARE(simple.paintRect(), QRectF(0, 0, 842, 595));
    QCOMPARE(simple.paintRect(QPageLayout::Millimeter), QRectF(0, 0, 297, 210));
    QCOMPARE(simple.paintRectPoints(), QRect(0, 0, 842, 595));
    QCOMPARE(simple.paintRectPixels(72), QRect(0, 0, 842, 595));

    // A4, 10pt margins
    QPageLayout tenpoint = QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait, QMarginsF(10, 10, 10, 10));
    QCOMPARE(tenpoint.isValid(), true);
    QCOMPARE(tenpoint.margins(), QMarginsF(10, 10, 10, 10));
    QCOMPARE(tenpoint.margins(QPageLayout::Millimeter), QMarginsF(3.53, 3.53, 3.53, 3.53));
    QCOMPARE(tenpoint.marginsPoints(), QMargins(10, 10, 10, 10));
    QCOMPARE(tenpoint.marginsPixels(72), QMargins(10, 10, 10, 10));
    QCOMPARE(tenpoint.minimumMargins(), QMarginsF(0, 0, 0, 0));
    QCOMPARE(tenpoint.maximumMargins(), QMarginsF(595, 842, 595, 842));
    QCOMPARE(tenpoint.fullRect(), QRectF(0, 0, 595, 842));
    QCOMPARE(tenpoint.fullRect(QPageLayout::Millimeter), QRectF(0, 0, 210, 297));
    QCOMPARE(tenpoint.fullRectPoints(), QRect(0, 0, 595, 842));
    QCOMPARE(tenpoint.fullRectPixels(72), QRect(0, 0, 595, 842));
    QCOMPARE(tenpoint.paintRect(), QRectF(10, 10, 575, 822));
    QCOMPARE(tenpoint.paintRect(QPageLayout::Millimeter), QRectF(3.53, 3.53, 202.94, 289.94));
    QCOMPARE(tenpoint.paintRect(QPageLayout::Millimeter).x(), 3.53);
    QCOMPARE(tenpoint.paintRect(QPageLayout::Millimeter).y(), 3.53);
    QCOMPARE(tenpoint.paintRect(QPageLayout::Millimeter).width(), 202.94);
    QCOMPARE(tenpoint.paintRect(QPageLayout::Millimeter).height(), 289.94);
    QCOMPARE(tenpoint.paintRect(QPageLayout::Millimeter).left(), 3.53);
    QCOMPARE(tenpoint.paintRect(QPageLayout::Millimeter).right(), 206.47);
    QCOMPARE(tenpoint.paintRect(QPageLayout::Millimeter).top(), 3.53);
    QCOMPARE(tenpoint.paintRect(QPageLayout::Millimeter).bottom(), 293.47);
    QCOMPARE(tenpoint.paintRectPoints(), QRect(10, 10, 575, 822));
    QCOMPARE(tenpoint.paintRectPixels(72), QRect(10, 10, 575, 822));

    // Change orientation
    tenpoint.setOrientation(QPageLayout::Landscape);
    QCOMPARE(tenpoint.orientation(), QPageLayout::Landscape);
    QCOMPARE(tenpoint.margins(), QMarginsF(10, 10, 10, 10));
    QCOMPARE(tenpoint.minimumMargins(), QMarginsF(0, 0, 0, 0));
    QCOMPARE(tenpoint.maximumMargins(), QMarginsF(842, 595, 842, 595));
    QCOMPARE(tenpoint.fullRect(), QRectF(0, 0, 842, 595));
    QCOMPARE(tenpoint.fullRect(QPageLayout::Millimeter), QRectF(0, 0, 297, 210));
    QCOMPARE(tenpoint.fullRectPoints(), QRect(0, 0, 842, 595));
    QCOMPARE(tenpoint.fullRectPixels(72), QRect(0, 0, 842, 595));
    QCOMPARE(tenpoint.paintRect(), QRectF(10, 10, 822, 575));
    QCOMPARE(tenpoint.paintRect(QPageLayout::Millimeter), QRectF(3.53, 3.53, 289.94, 202.94));
    QCOMPARE(tenpoint.paintRectPoints(), QRect(10, 10, 822, 575));
    QCOMPARE(tenpoint.paintRectPixels(72), QRect(10, 10, 822, 575));

    // Change mode
    QCOMPARE(tenpoint.mode(), QPageLayout::StandardMode);
    tenpoint.setMode(QPageLayout::FullPageMode);
    QCOMPARE(tenpoint.mode(), QPageLayout::FullPageMode);
    QCOMPARE(tenpoint.orientation(), QPageLayout::Landscape);
    QCOMPARE(tenpoint.margins(), QMarginsF(10, 10, 10, 10));
    QCOMPARE(tenpoint.minimumMargins(), QMarginsF(0, 0, 0, 0));
    QCOMPARE(tenpoint.maximumMargins(), QMarginsF(842, 595, 842, 595));
    QCOMPARE(tenpoint.fullRect(), QRectF(0, 0, 842, 595));
    QCOMPARE(tenpoint.fullRect(QPageLayout::Millimeter), QRectF(0, 0, 297, 210));
    QCOMPARE(tenpoint.fullRectPoints(), QRect(0, 0, 842, 595));
    QCOMPARE(tenpoint.fullRectPixels(72), QRect(0, 0, 842, 595));
    QCOMPARE(tenpoint.paintRect(), QRectF(0, 0, 842, 595));
    QCOMPARE(tenpoint.paintRect(QPageLayout::Millimeter), QRectF(0, 0, 297, 210));
    QCOMPARE(tenpoint.paintRectPoints(), QRect(0, 0, 842, 595));
    QCOMPARE(tenpoint.paintRectPixels(72), QRect(0, 0, 842, 595));
}

void tst_QPageLayout::setGetMargins()
{
    // A4, 20pt margins
    QMarginsF margins = QMarginsF(10, 10, 10, 10);
    QMarginsF min = QMarginsF(10, 10, 10, 10);
    QMarginsF max = QMarginsF(585, 832, 585, 832);
    QPageLayout change = QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait, margins, QPageLayout::Point, min);
    QCOMPARE(change.isValid(), true);
    QCOMPARE(change.margins(), margins);
    QCOMPARE(change.margins(QPageLayout::Millimeter), QMarginsF(3.53, 3.53, 3.53, 3.53));
    QCOMPARE(change.marginsPoints(), QMargins(10, 10, 10, 10));
    QCOMPARE(change.marginsPixels(72), QMargins(10, 10, 10, 10));
    QCOMPARE(change.minimumMargins(), min);
    QCOMPARE(change.maximumMargins(), max);

    // Set magins within min/max ok
    margins = QMarginsF(20, 20, 20, 20);
    change.setMargins(margins);
    QCOMPARE(change.margins(QPageLayout::Millimeter), QMarginsF(7.06, 7.06, 7.06, 7.06));
    QCOMPARE(change.marginsPoints(), QMargins(20, 20, 20, 20));
    QCOMPARE(change.marginsPixels(72), QMargins(20, 20, 20, 20));
    QCOMPARE(change.margins(), margins);

    // Set margins all below min is rejected
    change.setMargins(QMarginsF(0, 0, 0, 0));
    QCOMPARE(change.margins(), margins);

    // Set margins all above max is rejected
    change.setMargins(QMarginsF(1000, 1000, 1000, 1000));
    QCOMPARE(change.margins(), margins);

    // Only 1 wrong, set still rejects
    change.setMargins(QMarginsF(50, 50, 50, 0));
    QCOMPARE(change.margins(), margins);

    // Set page size resets min/max, clamps existing margins
    change.setMargins(change.maximumMargins());
    change.setPageSize(QPageSize(QPageSize::A5));
    QCOMPARE(change.margins(), QMarginsF(420, 595, 420, 595));
    QCOMPARE(change.minimumMargins(), QMarginsF(0, 0, 0, 0));
    QCOMPARE(change.maximumMargins(), QMarginsF(420, 595, 420, 595));

    // Set page size, sets min/max, clamps existing margins
    margins = QMarginsF(20, 500, 20, 500);
    change.setMargins(margins);
    QCOMPARE(change.margins(), margins);
    min = QMarginsF(30, 30, 30, 30);
    max = QMarginsF(267, 390, 267, 390);
    change.setPageSize(QPageSize(QPageSize::A6));
    change.setMinimumMargins(min);
    QCOMPARE(change.margins(), QMarginsF(30, 390, 30, 390));
    QCOMPARE(change.minimumMargins(), min);
    QCOMPARE(change.maximumMargins(), max);

    // A4, 20pt margins
    margins = QMarginsF(20, 20, 20, 20);
    min = QMarginsF(10, 10, 10, 10);
    max = QMarginsF(585, 832, 585, 832);
    QPageLayout fullPage = QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait, margins, QPageLayout::Point, min);
    fullPage.setMode(QPageLayout::FullPageMode);
    QCOMPARE(fullPage.isValid(), true);
    QCOMPARE(fullPage.margins(), margins);
    QCOMPARE(fullPage.minimumMargins(), min);
    QCOMPARE(fullPage.maximumMargins(), max);

    // Set margins within min/max ok
    margins = QMarginsF(50, 50, 50, 50);
    fullPage.setMargins(margins);
    QCOMPARE(fullPage.margins(), margins);

    // Set margins all below min is accepted
    margins = QMarginsF(0, 0, 0, 0);
    fullPage.setMargins(margins);
    QCOMPARE(fullPage.margins(), margins);

    // Set margins all above max is rejected
    margins = QMarginsF(1000, 1000, 1000, 1000);
    fullPage.setMargins(margins);
    QCOMPARE(fullPage.margins(), margins);

    // Only 1 wrong, set still accepts
    margins = QMarginsF(50, 50, 50, 0);
    fullPage.setMargins(margins);
    QCOMPARE(fullPage.margins(), margins);

    // Set page size, sets min/max, clamps existing margins
    margins = QMarginsF(20, 500, 20, 500);
    fullPage.setMargins(margins);
    QCOMPARE(fullPage.margins(), margins);
    min = QMarginsF(30, 30, 30, 30);
    max = QMarginsF(267, 390, 267, 390);
    fullPage.setPageSize(QPageSize(QPageSize::A6));
    fullPage.setMinimumMargins(min);
    QCOMPARE(fullPage.margins(), margins);
    QCOMPARE(fullPage.minimumMargins(), min);
    QCOMPARE(fullPage.maximumMargins(), max);
}

QTEST_APPLESS_MAIN(tst_QPageLayout)

#include "tst_qpagelayout.moc"
