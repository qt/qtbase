// Copyright (C) 2014 John Layt <jlayt@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtGui/qpagelayout.h>

class tst_QPageLayout : public QObject
{
    Q_OBJECT

private slots:
    void invalid();
    void basics();
    void setGetMargins();
    void setGetClampedMargins();
    void setUnits_data();
    void setUnits();
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
    QCOMPARE(a4portrait, simple);

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
    QCOMPARE(tenpoint.marginsPixels(600), QMargins(83, 83, 83, 83));
    QCOMPARE(tenpoint.minimumMargins(), QMarginsF(0, 0, 0, 0));
    QCOMPARE(tenpoint.maximumMargins(), QMarginsF(595, 842, 595, 842));
    QCOMPARE(tenpoint.fullRect(), QRectF(0, 0, 595, 842));
    QCOMPARE(tenpoint.fullRect(QPageLayout::Millimeter), QRectF(0, 0, 210, 297));
    QCOMPARE(tenpoint.fullRectPoints(), QRect(0, 0, 595, 842));
    QCOMPARE(tenpoint.fullRectPixels(72), QRect(0, 0, 595, 842));
    QCOMPARE(tenpoint.fullRectPixels(600), QRect(0, 0, 4958, 7016));
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
    QCOMPARE(tenpoint.paintRectPixels(600), QRect(83, 83, 4792, 6850));

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
    QCOMPARE(tenpoint.fullRectPixels(600), QRect(0, 0, 7016, 4958));
    QCOMPARE(tenpoint.paintRect(), QRectF(10, 10, 822, 575));
    QCOMPARE(tenpoint.paintRect(QPageLayout::Millimeter), QRectF(3.53, 3.53, 289.94, 202.94));
    QCOMPARE(tenpoint.paintRectPoints(), QRect(10, 10, 822, 575));
    QCOMPARE(tenpoint.paintRectPixels(72), QRect(10, 10, 822, 575));
    QCOMPARE(tenpoint.paintRectPixels(600), QRect(83, 83, 6850, 4792));

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
    QCOMPARE(tenpoint.fullRectPixels(600), QRect(0, 0, 7016, 4958));
    QCOMPARE(tenpoint.paintRect(), QRectF(0, 0, 842, 595));
    QCOMPARE(tenpoint.paintRect(QPageLayout::Millimeter), QRectF(0, 0, 297, 210));
    QCOMPARE(tenpoint.paintRectPoints(), QRect(0, 0, 842, 595));
    QCOMPARE(tenpoint.paintRectPixels(72), QRect(0, 0, 842, 595));
    QCOMPARE(tenpoint.paintRectPixels(600), QRect(0, 0, 7016, 4958));

    // A4, 8.4pt margins
    QPageLayout fraction = QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait, QMarginsF(8.4, 8.4, 8.4, 8.4));
    QCOMPARE(fraction.isValid(), true);
    QCOMPARE(fraction.margins(), QMarginsF(8.4, 8.4, 8.4, 8.4));
    QCOMPARE(fraction.margins(QPageLayout::Millimeter), QMarginsF(2.96, 2.96, 2.96, 2.96));
    QCOMPARE(fraction.marginsPoints(), QMarginsF(8, 8, 8, 8));
    QCOMPARE(fraction.marginsPixels(72), QMargins(8, 8, 8, 8));
    QCOMPARE(fraction.marginsPixels(600), QMargins(70, 70, 70, 70));
    QCOMPARE(fraction.minimumMargins(), QMarginsF(0, 0, 0, 0));
    QCOMPARE(fraction.maximumMargins(), QMarginsF(595, 842, 595, 842));
    QCOMPARE(fraction.fullRect(), QRectF(0, 0, 595, 842));
    QCOMPARE(fraction.fullRect(QPageLayout::Millimeter), QRectF(0, 0, 210, 297));
    QCOMPARE(fraction.fullRectPoints(), QRect(0, 0, 595, 842));
    QCOMPARE(fraction.fullRectPixels(72), QRect(0, 0, 595, 842));
    QCOMPARE(fraction.fullRectPixels(600), QRect(0, 0, 4958, 7016));
    QCOMPARE(fraction.paintRect(), QRectF(8.4, 8.4, 578.2, 825.2));
    QCOMPARE(fraction.paintRect(QPageLayout::Millimeter), QRectF(2.96, 2.96, 204.08, 291.08));
    QCOMPARE(fraction.paintRect(QPageLayout::Millimeter).x(), 2.96);
    QCOMPARE(fraction.paintRect(QPageLayout::Millimeter).y(), 2.96);
    QCOMPARE(fraction.paintRect(QPageLayout::Millimeter).width(), 204.08);
    QCOMPARE(fraction.paintRect(QPageLayout::Millimeter).height(), 291.08);
    QCOMPARE(fraction.paintRect(QPageLayout::Millimeter).left(), 2.96);
    QCOMPARE(fraction.paintRect(QPageLayout::Millimeter).right(), 207.04);
    QCOMPARE(fraction.paintRect(QPageLayout::Millimeter).top(), 2.96);
    QCOMPARE(fraction.paintRect(QPageLayout::Millimeter).bottom(), 294.04);
    QCOMPARE(fraction.paintRectPoints(), QRect(8, 8, 579, 826));
    QCOMPARE(fraction.paintRectPixels(72), QRect(8, 8, 579, 826));
    QCOMPARE(fraction.paintRectPixels(600), QRect(70, 70, 4818, 6876));

    // Change orientation
    fraction.setOrientation(QPageLayout::Landscape);
    QCOMPARE(fraction.orientation(), QPageLayout::Landscape);
    QCOMPARE(fraction.margins(), QMarginsF(8.4, 8.4, 8.4, 8.4));
    QCOMPARE(fraction.minimumMargins(), QMarginsF(0, 0, 0, 0));
    QCOMPARE(fraction.maximumMargins(), QMarginsF(842, 595, 842, 595));
    QCOMPARE(fraction.fullRect(), QRectF(0, 0, 842, 595));
    QCOMPARE(fraction.fullRect(QPageLayout::Millimeter), QRectF(0, 0, 297, 210));
    QCOMPARE(fraction.fullRectPoints(), QRect(0, 0, 842, 595));
    QCOMPARE(fraction.fullRectPixels(72), QRect(0, 0, 842, 595));
    QCOMPARE(fraction.fullRectPixels(600), QRect(0, 0, 7016, 4958));
    QCOMPARE(fraction.paintRect(), QRectF(8.4, 8.4, 825.2, 578.2));
    QCOMPARE(fraction.paintRect(QPageLayout::Millimeter), QRectF(2.96, 2.96, 291.08, 204.08));
    QCOMPARE(fraction.paintRectPoints(), QRect(8, 8, 826, 579));
    QCOMPARE(fraction.paintRectPixels(72), QRect(8, 8, 826, 579));
    QCOMPARE(fraction.paintRectPixels(600), QRect(70, 70, 6876, 4818));

    // Change mode
    QCOMPARE(fraction.mode(), QPageLayout::StandardMode);
    fraction.setMode(QPageLayout::FullPageMode);
    QCOMPARE(fraction.mode(), QPageLayout::FullPageMode);
    QCOMPARE(fraction.orientation(), QPageLayout::Landscape);
    QCOMPARE(fraction.margins(), QMarginsF(8.4, 8.4, 8.4, 8.4));
    QCOMPARE(fraction.minimumMargins(), QMarginsF(0, 0, 0, 0));
    QCOMPARE(fraction.maximumMargins(), QMarginsF(842, 595, 842, 595));
    QCOMPARE(fraction.fullRect(), QRectF(0, 0, 842, 595));
    QCOMPARE(fraction.fullRect(QPageLayout::Millimeter), QRectF(0, 0, 297, 210));
    QCOMPARE(fraction.fullRectPoints(), QRect(0, 0, 842, 595));
    QCOMPARE(fraction.fullRectPixels(72), QRect(0, 0, 842, 595));
    QCOMPARE(fraction.fullRectPixels(600), QRect(0, 0, 7016, 4958));
    QCOMPARE(fraction.paintRect(), QRectF(0, 0, 842, 595));
    QCOMPARE(fraction.paintRect(QPageLayout::Millimeter), QRectF(0, 0, 297, 210));
    QCOMPARE(fraction.paintRectPoints(), QRect(0, 0, 842, 595));
    QCOMPARE(fraction.paintRectPixels(72), QRect(0, 0, 842, 595));
    QCOMPARE(fraction.paintRectPixels(600), QRect(0, 0, 7016, 4958));
}

void tst_QPageLayout::setGetMargins()
{
    // A4, 10pt margins
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

    // Set margins within min/max ok
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

    // Set margins all above max is accepted
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

void tst_QPageLayout::setGetClampedMargins()
{
    // A4, 10pt margins
    QMarginsF margins = QMarginsF(10, 10, 10, 10);
    QMarginsF min = QMarginsF(10, 10, 10, 10);
    QMarginsF max = QMarginsF(585, 832, 585, 832);
    QPageLayout change = QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait, margins, QPageLayout::Point, min);
    QCOMPARE(change.isValid(), true);

    // Clamp margins within min/max ok
    margins = QMarginsF(20, 20, 20, 20);
    change.setMargins(margins, QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(change.margins(QPageLayout::Millimeter), QMarginsF(7.06, 7.06, 7.06, 7.06));
    QCOMPARE(change.marginsPoints(), QMargins(20, 20, 20, 20));
    QCOMPARE(change.marginsPixels(72), QMargins(20, 20, 20, 20));
    QCOMPARE(change.margins(), margins);

    // Clamp margins all below min
    change.setMargins(QMarginsF(0, 0, 0, 0), QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(change.margins(), change.minimumMargins());

    // Clamp margins all above max
    change.setMargins(QMarginsF(1000, 1000, 1000, 1000), QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(change.margins(), change.maximumMargins());

    // Only 1 wrong, clamp still works
    change.setMargins(QMarginsF(50, 50, 50, 0), QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(change.margins(), QMarginsF(50, 50, 50, change.minimumMargins().bottom()));

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

    // Clamp margins within min/max ok
    margins = QMarginsF(50, 50, 50, 50);
    fullPage.setMargins(margins, QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(fullPage.margins(), margins);

    // Clamp margins all below min, no clamping
    margins = QMarginsF(0, 0, 0, 0);
    fullPage.setMargins(margins, QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(fullPage.margins(), margins);

    // Clamp margins all above max, no clamping
    margins = QMarginsF(1000, 1000, 1000, 1000);
    fullPage.setMargins(margins, QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(fullPage.margins(), margins);

    // Only 1 wrong, no clamping
    margins = QMarginsF(50, 50, 50, 0);
    fullPage.setMargins(margins, QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(fullPage.margins(), margins);

    // Set page size, sets min/max, clamps existing margins
    margins = QMarginsF(20, 500, 20, 500);
    fullPage.setMargins(margins, QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(fullPage.margins(), margins);
    min = QMarginsF(30, 30, 30, 30);
    max = QMarginsF(267, 390, 267, 390);
    fullPage.setPageSize(QPageSize(QPageSize::A6));
    fullPage.setMinimumMargins(min);
    QCOMPARE(fullPage.margins(), margins);
    QCOMPARE(fullPage.minimumMargins(), min);
    QCOMPARE(fullPage.maximumMargins(), max);

    // Test set* API calls
    min = QMarginsF(1, 2, 3, 4);
    max = QMarginsF(595 - min.right(), 842 - min.bottom(), 595 - min.left(), 842 - min.top());
    change = QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait, margins, QPageLayout::Point, min);
    QCOMPARE(change.minimumMargins(), min);
    QCOMPARE(change.maximumMargins(), max);

    // Test setLeftMargin
    change.setLeftMargin(0, QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(change.margins().left(), min.left());
    change.setLeftMargin(change.fullRectPoints().width(), QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(change.margins().left(), max.left());
    change.setLeftMargin(min.left(), QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(change.margins().left(), min.left());
    change.setLeftMargin(max.left(), QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(change.margins().left(), max.left());

    // Test setTopMargin
    change.setTopMargin(0, QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(change.margins().top(), min.top());
    change.setTopMargin(change.fullRectPoints().height(), QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(change.margins().top(), max.top());
    change.setTopMargin(min.top(), QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(change.margins().top(), min.top());
    change.setTopMargin(max.top(), QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(change.margins().top(), max.top());

    // Test setRightMargin
    change.setRightMargin(0, QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(change.margins().right(), min.right());
    change.setRightMargin(change.fullRectPoints().width(), QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(change.margins().right(), max.right());
    change.setRightMargin(min.right(), QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(change.margins().right(), min.right());
    change.setRightMargin(max.right(), QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(change.margins().right(), max.right());

    // Test setBottomMargin
    change.setBottomMargin(0, QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(change.margins().bottom(), min.bottom());
    change.setBottomMargin(change.fullRectPoints().height(), QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(change.margins().bottom(), max.bottom());
    change.setBottomMargin(min.bottom(), QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(change.margins().bottom(), min.bottom());
    change.setBottomMargin(max.bottom(), QPageLayout::OutOfBoundsPolicy::Clamp);
    QCOMPARE(change.margins().bottom(), max.bottom());
}

void tst_QPageLayout::setUnits_data()
{
    QTest::addColumn<QPageLayout::Unit>("units");
    QTest::newRow("Millimeter") << QPageLayout::Millimeter;
    QTest::newRow("Point") << QPageLayout::Point;
    QTest::newRow("Inch") << QPageLayout::Inch;
    QTest::newRow("Pica") << QPageLayout::Pica;
    QTest::newRow("Didot") << QPageLayout::Didot;
    QTest::newRow("Cicero") << QPageLayout::Cicero;
}

void tst_QPageLayout::setUnits()
{
    QFETCH(QPageLayout::Unit, units);
    QPageLayout pageLayout = QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait, QMarginsF(), units);
    int maxLeftX100 = qFloor(pageLayout.maximumMargins().left() * 100);
    QVERIFY(maxLeftX100 > 0);
    for (int i = 1; i <= maxLeftX100; ++i) {
        const qreal margin = i / 100.;
        const QMarginsF unitsMargins = QMarginsF(margin, margin, margin, margin);
        pageLayout.setMargins(unitsMargins);
        pageLayout.setUnits(QPageLayout::Point);
        const QMarginsF pointsMargins = pageLayout.margins();
        if (units == QPageLayout::Point) {
            QCOMPARE(pointsMargins, unitsMargins);
        } else {
            QCOMPARE_GT(pointsMargins.left(), unitsMargins.left());
            QCOMPARE_GT(pointsMargins.top(), unitsMargins.top());
            QCOMPARE_GT(pointsMargins.right(), unitsMargins.right());
            QCOMPARE_GT(pointsMargins.bottom(), unitsMargins.bottom());
        }
        pageLayout.setUnits(units);
        const QMarginsF convertedUnitsMargins = pageLayout.margins();
        if (units == QPageLayout::Didot) {
            // When using Didot units, the small multiplier and ceiling function in conversion
            // may cause the converted units to not match the original exactly. However, we
            // can verify that the converted margins are always greater than or equal to the
            // original.
            QCOMPARE_GE(convertedUnitsMargins.left(), unitsMargins.left());
            QCOMPARE_GE(convertedUnitsMargins.top(), unitsMargins.top());
            QCOMPARE_GE(convertedUnitsMargins.right(), unitsMargins.right());
            QCOMPARE_GE(convertedUnitsMargins.bottom(), unitsMargins.bottom());
        } else {
            QCOMPARE(convertedUnitsMargins, unitsMargins);
        }
    }
}

QTEST_APPLESS_MAIN(tst_QPageLayout)

#include "tst_qpagelayout.moc"
