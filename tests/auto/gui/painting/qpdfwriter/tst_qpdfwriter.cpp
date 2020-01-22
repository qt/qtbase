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

#include <QtTest/QtTest>
#include <QtGlobal>
#include <QtAlgorithms>
#include <QtGui/QAbstractTextDocumentLayout>
#include <QtGui/QPageLayout>
#include <QtGui/QPdfWriter>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>

class tst_QPdfWriter : public QObject
{
    Q_OBJECT

private slots:
    void basics();
    void testPageMetrics_data();
    void testPageMetrics();
    void qtbug59443();
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

    writer.setPageSize(QPageSize(QPageSize::A3));
    QCOMPARE(writer.pageLayout().pageSize().id(), QPageSize::A3);
    QCOMPARE(writer.pageSize(), QPdfWriter::A3);
    QCOMPARE(writer.pageSizeMM(), QSizeF(297, 420));

    writer.setPageSize(QPageSize(QSize(210, 297), QPageSize::Millimeter));
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
    const QMarginsF margins = {50, 50, 50, 50};
    writer.setPageMargins(margins, QPageLayout::Millimeter);
    QCOMPARE(writer.pageLayout().margins(), margins);
    QCOMPARE(writer.pageLayout().units(), QPageLayout::Millimeter);
    QCOMPARE(writer.margins().left, 50.0);
    QCOMPARE(writer.margins().right, 50.0);
    QCOMPARE(writer.margins().top, 50.0);
    QCOMPARE(writer.margins().bottom, 50.0);

    QCOMPARE(writer.pageLayout().fullRect(QPageLayout::Millimeter), QRectF(0, 0, 297, 210));
    QCOMPARE(writer.pageLayout().paintRect(QPageLayout::Millimeter), QRectF(50, 50, 197, 110));

    QByteArray metadata (
            "<?xpacket begin='' id='W5M0MpCehiHzreSzNTczkc9d'?>\n"
            "<x:xmpmeta xmlns:x=\"adobe:ns:meta/\">\n"
            "  <rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\"\n"
            "    <rdf:Description xmlns:dc=\"http://purl.org/dc/elements/1.1/\" rdf:about=\"\">\n"
            "      <dc:title>\n"
            "        <rdf:Alt>\n"
            "          <rdf:li xml:lang=\"x-default\">TITLE</rdf:li>\n"
            "        </rdf:Alt>\n"
            "      </dc:title>\n"
            "    </rdf:Description>\n"
            "    <rdf:Description xmlns:xmp=\"http://ns.adobe.com/xap/1.0/\" rdf:about="" xmp:CreatorTool=\"OUR_OWN_XMP\" xmp:CreateDate=\"2019-12-16T00:00:00+01:00\" xmp:ModifyDate=\"2019-12-16T00:00:00+01:00\"/>\n"
            "    <rdf:Description xmlns:pdf=\"http://ns.adobe.com/pdf/1.3/\" rdf:about="" pdf:Producer=\"MetaType Info Producer\"/>\n"
            "    <rdf:Description xmlns:pdfaid=\"http://www.aiim.org/pdfa/ns/id/\" rdf:about=\"THI IS ALL ABOUT\" pdfaid:part=\"1\" pdfaid:conformance=\"B\"/>\n"
            "  </rdf:RDF>\n"
            "</x:xmpmeta>\n"
            "<?xpacket end='w'?>\n"
    );

    QCOMPARE(writer.documentXmpMetadata(), QByteArray());
    writer.setDocumentXmpMetadata(metadata);
    QCOMPARE(writer.documentXmpMetadata(), metadata);
}

// Test the old page metrics methods, see also QPrinter tests for the same.
void tst_QPdfWriter::testPageMetrics_data()
{
    QTest::addColumn<QPageSize::PageSizeId>("pageSizeId");
    QTest::addColumn<qreal>("widthMMf");
    QTest::addColumn<qreal>("heightMMf");
    QTest::addColumn<bool>("setMargins");
    QTest::addColumn<qreal>("leftMMf");
    QTest::addColumn<qreal>("rightMMf");
    QTest::addColumn<qreal>("topMMf");
    QTest::addColumn<qreal>("bottomMMf");

    QTest::newRow("A4")         << QPageSize::A4 << 210.0 << 297.0 << false
                                << 3.53 <<  3.53 << 3.53 <<  3.53;
    QTest::newRow("A4 Margins") << QPageSize::A4 << 210.0 << 297.0 << true
                                << 20.0 << 30.0  << 40.0  << 50.0;

    QTest::newRow("Portrait")          << QPageSize::Custom << 345.0 << 678.0 << false
                                       << 3.53 << 3.53 << 3.53 <<  3.53;
    QTest::newRow("Portrait Margins")  << QPageSize::Custom << 345.0 << 678.0 << true
                                       << 20.0 << 30.0 << 40.0 << 50.0;
    QTest::newRow("Landscape")         << QPageSize::Custom << 678.0 << 345.0 << false
                                       << 3.53 << 3.53 << 3.53 << 3.53;
    QTest::newRow("Landscape Margins") << QPageSize::Custom << 678.0 << 345.0 << true
                                       << 20.0 << 30.0 << 40.0 << 50.0;
}

void tst_QPdfWriter::testPageMetrics()
{
    QFETCH(QPageSize::PageSizeId, pageSizeId);
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
        writer.setPageMargins({leftMMf, topMMf, rightMMf, bottomMMf}, QPageLayout::Millimeter);
        QCOMPARE(writer.margins().left, leftMMf);
        QCOMPARE(writer.margins().right, rightMMf);
        QCOMPARE(writer.margins().top, topMMf);
        QCOMPARE(writer.margins().bottom, bottomMMf);
    }

    // Set the given size, in Portrait mode
    const QPageSize pageSize = pageSizeId == QPageSize::Custom
        ? QPageSize(sizeMMf, QPageSize::Millimeter) : QPageSize(pageSizeId);
    writer.setPageSize(pageSize);
    QCOMPARE(writer.pageLayout().pageSize().id(), pageSizeId);
    QCOMPARE(int(writer.pageSize()), int(pageSizeId));

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
    QCOMPARE(writer.pageLayout().pageSize().id(), pageSizeId);
    QCOMPARE(int(writer.pageSize()), int(pageSizeId));
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
    writer.setPageSize(pageSize);
    QCOMPARE(writer.pageLayout().pageSize().id(), pageSizeId);
    QCOMPARE(int(writer.pageSize()), int(pageSizeId));
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

void tst_QPdfWriter::qtbug59443()
{
    // Do not crash or assert
    QTemporaryFile file;
    QVERIFY2(file.open(), qPrintable(file.errorString()));
    QPdfWriter writer(file.fileName());
    writer.setPageSize(QPageSize(QPageSize::A4));
    QTextDocument doc;
    doc.documentLayout()->setPaintDevice(&writer);

    doc.setUndoRedoEnabled(false);
    QTextCursor cursor(&doc);
    QFont font = doc.defaultFont();
    font.setFamily("Calibri");
    font.setPointSize(8);
    doc.setDefaultFont(font);

    cursor.insertText(QString::fromStdWString(L"기초하며, 베어링제조업체와 타\n"));
    doc.print(&writer);

}

QTEST_MAIN(tst_QPdfWriter)

#include "tst_qpdfwriter.moc"
