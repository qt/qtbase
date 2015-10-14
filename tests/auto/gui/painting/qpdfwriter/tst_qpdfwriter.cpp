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
#include <QtGlobal>
#include <QtAlgorithms>
#include <QtGui/QPageLayout>
#include <QtGui/QPdfWriter>

class tst_QPdfWriter : public QObject
{
    Q_OBJECT

private slots:
    void basics();
    void testPageMetrics_data();
    void testPageMetrics();
};

void tst_QPdfWriter::basics()
{
    QTemporaryFile file;
    QVERIFY2(file.open(), qPrintable(file.errorString()));
    QPdfWriter writer(file.fileName());

    QCOMPARE(writer.title(), QString());
    writer.setTitle(QString("Test Title"));
    QCOMPARE(writer.title(), QString("Test Title"));

    QCOMPARE(writer.creator(), QString());
    writer.setCreator(QString("Test Creator"));
    QCOMPARE(writer.creator(), QString("Test Creator"));

    QCOMPARE(writer.resolution(), 1200);
    writer.setResolution(600);
    QCOMPARE(writer.resolution(), 600);

    QCOMPARE(writer.pageLayout().pageSize().id(), QPageSize::A4);
    QCOMPARE(writer.pageSize(), QPdfWriter::A4);
    QCOMPARE(writer.pageSizeMM(), QSizeF(210, 297));

    writer.setPageSize(QPageSize(QPageSize::A5));
    QCOMPARE(writer.pageLayout().pageSize().id(), QPageSize::A5);
    QCOMPARE(writer.pageSize(), QPdfWriter::A5);
    QCOMPARE(writer.pageSizeMM(), QSizeF(148, 210));

    writer.setPageSize(QPdfWriter::A3);
    QCOMPARE(writer.pageLayout().pageSize().id(), QPageSize::A3);
    QCOMPARE(writer.pageSize(), QPdfWriter::A3);
    QCOMPARE(writer.pageSizeMM(), QSizeF(297, 420));

    writer.setPageSizeMM(QSize(210, 297));
    QCOMPARE(writer.pageLayout().pageSize().id(), QPageSize::A4);
    QCOMPARE(writer.pageSize(), QPdfWriter::A4);
    QCOMPARE(writer.pageSizeMM(), QSizeF(210, 297));

    QCOMPARE(writer.pageLayout().orientation(), QPageLayout::Portrait);
    writer.setPageOrientation(QPageLayout::Landscape);
    QCOMPARE(writer.pageLayout().orientation(), QPageLayout::Landscape);
    QCOMPARE(writer.pageSizeMM(), QSizeF(210, 297));

    QCOMPARE(writer.pageLayout().margins(), QMarginsF(10, 10, 10, 10));
    QCOMPARE(writer.pageLayout().units(), QPageLayout::Point);
    QCOMPARE(writer.margins().left, 3.53);  // mm
    QCOMPARE(writer.margins().right, 3.53);
    QCOMPARE(writer.margins().top, 3.53);
    QCOMPARE(writer.margins().bottom, 3.53);
    writer.setPageMargins(QMarginsF(20, 20, 20, 20), QPageLayout::Millimeter);
    QCOMPARE(writer.pageLayout().margins(), QMarginsF(20, 20, 20, 20));
    QCOMPARE(writer.pageLayout().units(), QPageLayout::Millimeter);
    QCOMPARE(writer.margins().left, 20.0);
    QCOMPARE(writer.margins().right, 20.0);
    QCOMPARE(writer.margins().top, 20.0);
    QCOMPARE(writer.margins().bottom, 20.0);
    QPdfWriter::Margins margins = {50, 50, 50, 50};
    writer.setMargins(margins);
    QCOMPARE(writer.pageLayout().margins(), QMarginsF(50, 50, 50, 50));
    QCOMPARE(writer.pageLayout().units(), QPageLayout::Millimeter);
    QCOMPARE(writer.margins().left, 50.0);
    QCOMPARE(writer.margins().right, 50.0);
    QCOMPARE(writer.margins().top, 50.0);
    QCOMPARE(writer.margins().bottom, 50.0);

    QCOMPARE(writer.pageLayout().fullRect(QPageLayout::Millimeter), QRectF(0, 0, 297, 210));
    QCOMPARE(writer.pageLayout().paintRect(QPageLayout::Millimeter), QRectF(50, 50, 197, 110));
}

// Test the old page metrics methods, see also QPrinter tests for the same.
void tst_QPdfWriter::testPageMetrics_data()
{
    QTest::addColumn<int>("pageSize");
    QTest::addColumn<qreal>("widthMMf");
    QTest::addColumn<qreal>("heightMMf");
    QTest::addColumn<bool>("setMargins");
    QTest::addColumn<qreal>("leftMMf");
    QTest::addColumn<qreal>("rightMMf");
    QTest::addColumn<qreal>("topMMf");
    QTest::addColumn<qreal>("bottomMMf");

    QTest::newRow("A4")                << int(QPdfWriter::A4) << 210.0 << 297.0 << false <<  3.53 <<  3.53 <<  3.53 <<  3.53;
    QTest::newRow("A4 Margins")        << int(QPdfWriter::A4) << 210.0 << 297.0 << true  << 20.0  << 30.0  << 40.0  << 50.0;
    QTest::newRow("Portrait")          << -1                  << 345.0 << 678.0 << false <<  3.53 <<  3.53 <<  3.53 <<  3.53;
    QTest::newRow("Portrait Margins")  << -1                  << 345.0 << 678.0 << true  << 20.0  << 30.0  << 40.0  << 50.0;
    QTest::newRow("Landscape")         << -1                  << 678.0 << 345.0 << false <<  3.53 <<  3.53 <<  3.53 <<  3.53;
    QTest::newRow("Landscape Margins") << -1                  << 678.0 << 345.0 << true  << 20.0  << 30.0  << 40.0  << 50.0;
}

void tst_QPdfWriter::testPageMetrics()
{
    QFETCH(int, pageSize);
    QFETCH(qreal, widthMMf);
    QFETCH(qreal, heightMMf);
    QFETCH(bool, setMargins);
    QFETCH(qreal, leftMMf);
    QFETCH(qreal, rightMMf);
    QFETCH(qreal, topMMf);
    QFETCH(qreal, bottomMMf);

    QSizeF sizeMMf = QSizeF(widthMMf, heightMMf);

    QTemporaryFile file;
    QVERIFY2(file.open(), qPrintable(file.errorString()));
    QPdfWriter writer(file.fileName());
    QCOMPARE(writer.pageLayout().orientation(), QPageLayout::Portrait);

    if (setMargins) {
        // Setup the given margins
        QPdfWriter::Margins margins;
        margins.left = leftMMf;
        margins.right = rightMMf;
        margins.top = topMMf;
        margins.bottom = bottomMMf;
        writer.setMargins(margins);
        QCOMPARE(writer.margins().left, leftMMf);
        QCOMPARE(writer.margins().right, rightMMf);
        QCOMPARE(writer.margins().top, topMMf);
        QCOMPARE(writer.margins().bottom, bottomMMf);
    }


    // Set the given size, in Portrait mode
    if (pageSize < 0) {
        writer.setPageSizeMM(sizeMMf);
        QCOMPARE(writer.pageSize(), QPdfWriter::Custom);
        QCOMPARE(writer.pageLayout().pageSize().id(), QPageSize::Custom);
    } else {
        writer.setPageSize(QPdfWriter::PageSize(pageSize));
        QCOMPARE(writer.pageSize(), QPdfWriter::PageSize(pageSize));
        QCOMPARE(writer.pageLayout().pageSize().id(), QPageSize::PageSizeId(pageSize));
    }
    QCOMPARE(writer.pageLayout().orientation(), QPageLayout::Portrait);
    QCOMPARE(writer.margins().left, leftMMf);
    QCOMPARE(writer.margins().right, rightMMf);
    QCOMPARE(writer.margins().top, topMMf);
    QCOMPARE(writer.margins().bottom, bottomMMf);

    // QPagedPaintDevice::pageSizeMM() always returns Portrait
    QCOMPARE(writer.pageSizeMM(), sizeMMf);

    // QPagedPaintDevice::widthMM() and heightMM() are paint metrics and always return set orientation
    QCOMPARE(writer.widthMM(), qRound(widthMMf - leftMMf - rightMMf));
    QCOMPARE(writer.heightMM(), qRound(heightMMf - topMMf - bottomMMf));

    // Now switch to Landscape mode, size should be unchanged, but rect and metrics should change
    writer.setPageOrientation(QPageLayout::Landscape);
    if (pageSize < 0) {
        QCOMPARE(writer.pageSize(), QPdfWriter::Custom);
        QCOMPARE(writer.pageLayout().pageSize().id(), QPageSize::Custom);
    } else {
        QCOMPARE(writer.pageSize(), QPdfWriter::PageSize(pageSize));
        QCOMPARE(writer.pageLayout().pageSize().id(), QPageSize::PageSizeId(pageSize));
    }
    QCOMPARE(writer.pageLayout().orientation(), QPageLayout::Landscape);
    QCOMPARE(writer.margins().left, leftMMf);
    QCOMPARE(writer.margins().right, rightMMf);
    QCOMPARE(writer.margins().top, topMMf);
    QCOMPARE(writer.margins().bottom, bottomMMf);

    // QPagedPaintDevice::pageSizeMM() always returns Portrait
    QCOMPARE(writer.pageSizeMM(), sizeMMf);

    // QPagedPaintDevice::widthMM() and heightMM() are paint metrics and always return set orientation
    QCOMPARE(writer.widthMM(), qRound(heightMMf - leftMMf - rightMMf));
    QCOMPARE(writer.heightMM(), qRound(widthMMf - topMMf - bottomMMf));

    // QPdfWriter::fullRect() always returns set orientation
    QCOMPARE(writer.pageLayout().fullRect(QPageLayout::Millimeter), QRectF(0, 0, heightMMf, widthMMf));

    // QPdfWriter::paintRect() always returns set orientation
    QCOMPARE(writer.pageLayout().paintRect(QPageLayout::Millimeter), QRectF(leftMMf, topMMf, heightMMf - leftMMf - rightMMf, widthMMf - topMMf - bottomMMf));


    // Now while in Landscape mode, set the size again, results should be the same
    if (pageSize < 0) {
        writer.setPageSizeMM(sizeMMf);
        QCOMPARE(writer.pageSize(), QPdfWriter::Custom);
        QCOMPARE(writer.pageLayout().pageSize().id(), QPageSize::Custom);
    } else {
        writer.setPageSize(QPdfWriter::PageSize(pageSize));
        QCOMPARE(writer.pageSize(), QPdfWriter::PageSize(pageSize));
        QCOMPARE(writer.pageLayout().pageSize().id(), QPageSize::PageSizeId(pageSize));
    }
    QCOMPARE(writer.pageLayout().orientation(), QPageLayout::Landscape);
    QCOMPARE(writer.margins().left, leftMMf);
    QCOMPARE(writer.margins().right, rightMMf);
    QCOMPARE(writer.margins().top, topMMf);
    QCOMPARE(writer.margins().bottom, bottomMMf);

    // QPagedPaintDevice::pageSizeMM() always returns Portrait
    QCOMPARE(writer.pageSizeMM(), sizeMMf);

    // QPagedPaintDevice::widthMM() and heightMM() are paint metrics and always return set orientation
    QCOMPARE(writer.widthMM(), qRound(heightMMf - leftMMf - rightMMf));
    QCOMPARE(writer.heightMM(), qRound(widthMMf - topMMf - bottomMMf));

    // QPdfWriter::fullRect() always returns set orientation
    QCOMPARE(writer.pageLayout().fullRect(QPageLayout::Millimeter), QRectF(0, 0, heightMMf, widthMMf));

    // QPdfWriter::paintRect() always returns set orientation
    QCOMPARE(writer.pageLayout().paintRect(QPageLayout::Millimeter), QRectF(leftMMf, topMMf, heightMMf - leftMMf - rightMMf, widthMMf - topMMf - bottomMMf));
}

QTEST_MAIN(tst_QPdfWriter)

#include "tst_qpdfwriter.moc"
