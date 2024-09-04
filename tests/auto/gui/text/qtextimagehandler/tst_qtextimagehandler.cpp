// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <QPainter>
#include <private/qtextimagehandler_p.h>

using namespace Qt::StringLiterals;

// #define DEBUG_WRITE_HTML

class tst_QTextImageHandler : public QObject
{
    Q_OBJECT

public:
    tst_QTextImageHandler();

private slots:
    void init();
    void cleanup();
    void cleanupTestCase();
    void loadAtNImages_data();
#ifndef QT_NO_TEXTHTMLPARSER
    void loadAtNImages();
    void customResourceSchema_data();
    void customResourceSchema();
    void maxWidth_data();
    void maxWidth();
#endif
};

struct MyTextDocument : public QTextDocument
{
    QVariant loadResource(int, const QUrl &name) override
    {
        retrievedScheme = name.scheme();
        return {};
    }
    QString retrievedScheme;
};

tst_QTextImageHandler::tst_QTextImageHandler()
{
}

void tst_QTextImageHandler::init()
{
}

void tst_QTextImageHandler::cleanup()
{
}

void tst_QTextImageHandler::cleanupTestCase()
{
}

void tst_QTextImageHandler::loadAtNImages_data()
{
    QTest::addColumn<QString>("imageFile");

    QTest::addRow("file") << QFINDTESTDATA("data/image.png");
    QTest::addRow("file_url") << QUrl::fromLocalFile(QFINDTESTDATA("data/image.png")).toString();
    QTest::addRow("resource") << ":/data/image.png";
    QTest::addRow("qrc_url") << "qrc:/data/image.png";
}

#ifndef QT_NO_TEXTHTMLPARSER
void tst_QTextImageHandler::loadAtNImages()
{
    QFETCH(QString, imageFile);

    QTextDocument doc;
    QTextCursor c(&doc);
    c.insertHtml("<img src=\"" + imageFile + "\">");
    const auto formats = doc.allFormats();
    const auto it = std::find_if(formats.begin(), formats.end(), [](const auto &format){
        return format.objectType() == QTextFormat::ImageObject;
    });
    QCOMPARE_NE(it, formats.end());
    const QTextImageFormat format = (*it).toImageFormat();
    QTextImageHandler handler;

    for (const auto &dpr : {1, 2}) {
        QImage img(20, 20, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::white);
        img.setDevicePixelRatio(dpr);
        QPainter p(&img);
        handler.drawObject(&p, QRect(0, 0, 20, 20), &doc, 0, format);
        p.end();
        QVERIFY(!img.isNull());
        const auto expectedColor = dpr == 1 ? Qt::red : Qt::green;
        QCOMPARE(img.pixelColor(0, 0), expectedColor);
    }
}

void tst_QTextImageHandler::customResourceSchema_data()
{
    QTest::addColumn<QString>("imageFile");
    QTest::addColumn<QString>("expectedScheme");

    QTest::addRow("file_url") << QUrl::fromLocalFile(QFINDTESTDATA("data/image.png")).toString() << "file";
    QTest::addRow("resource") << ":/data/image.png" << "qrc";
    QTest::addRow("qrc_url") << "qrc:/data/image.png" << "qrc";
    QTest::addRow("custom_url") << "custom:/data/image.png" << "custom";
}

void tst_QTextImageHandler::customResourceSchema()
{
    QFETCH(QString, imageFile);
    QFETCH(QString, expectedScheme);

    MyTextDocument doc;

    QTextCursor c(&doc);
    c.insertHtml("<img src=\"" + imageFile + "\">");
    const auto formats = doc.allFormats();
    const auto it = std::find_if(formats.begin(), formats.end(), [](const auto &format){
        return format.objectType() == QTextFormat::ImageObject;
    });
    QCOMPARE_NE(it, formats.end());
    QImage img(20, 20, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::white);
    QPainter p(&img);
    QTextImageHandler handler;
    handler.drawObject(&p, QRect(0, 0, 20, 20), &doc, 0, *it);
    p.end();
    QCOMPARE(expectedScheme, doc.retrievedScheme);
}

void tst_QTextImageHandler::maxWidth_data()
{
    QTest::addColumn<QString>("imageFile");
    QTest::addColumn<QSizeF>("pageSize");
    QTest::addColumn<QTextLength>("maxWidth");
    QTest::addColumn<QSizeF>("expectedSize");

    QTest::addRow("constrained-percentage") << QFINDTESTDATA("data/image.png") << QSizeF(16, 16) << QTextLength(QTextLength::PercentageLength, 100) << QSizeF(12, 12);
    QTest::addRow("not-constrained-percentage") << QFINDTESTDATA("data/image.png") << QSizeF(200, 200) << QTextLength(QTextLength::PercentageLength, 100) << QSizeF(16, 16);
    QTest::addRow("constrained-fixed") << QFINDTESTDATA("data/image.png") << QSizeF(16, 16) << QTextLength(QTextLength::FixedLength, 5) << QSizeF(5, 5);
    QTest::addRow("not-constrained-fixed") << QFINDTESTDATA("data/image.png") << QSizeF(200, 200) << QTextLength(QTextLength::FixedLength, 5) << QSizeF(5, 5);
    QTest::addRow("not-constrained-default") << QFINDTESTDATA("data/image.png") << QSizeF(200, 200) << QTextLength(QTextLength::VariableLength, 5) << QSizeF(16, 16);
}

void tst_QTextImageHandler::maxWidth()
{
    QFETCH(QString, imageFile);
    QFETCH(QSizeF, pageSize);
    QFETCH(QTextLength, maxWidth);
    QFETCH(QSizeF, expectedSize);

    QTextDocument doc;
    doc.setPageSize(pageSize);
    doc.setDocumentMargin(2);
    QTextCursor c(&doc);
    QString style;
    if (maxWidth.type() == QTextLength::PercentageLength)
        style = " style=\"max-width:"_L1 + QString::number(maxWidth.rawValue()) + "%;\""_L1;
    else if (maxWidth.type() == QTextLength::FixedLength)
        style = " style=\"max-width:"_L1 + QString::number(maxWidth.rawValue()) + "px;\""_L1;
    const QString html = "<img src=\"" + imageFile + u'\"' + style + "\">";
    c.insertHtml(html);

#ifdef DEBUG_WRITE_HTML
    {
        QFile out("/tmp/" + QLatin1String(QTest::currentDataTag()) + ".html");
        out.open(QFile::WriteOnly);
        out.write(html.toLatin1());
        out.close();
    }
    {
        QFile out("/tmp/" + QLatin1String(QTest::currentDataTag()) + "_rewrite.html");
        out.open(QFile::WriteOnly);
        out.write(doc.toHtml().toLatin1());
        out.close();
    }
#endif
    const auto formats = doc.allFormats();
    const auto it = std::find_if(formats.begin(), formats.end(), [](const auto &format){
        return format.objectType() == QTextFormat::ImageObject;
    });
    QCOMPARE_NE(it, formats.end());
    const QTextImageFormat format = (*it).toImageFormat();
    QTextImageHandler handler;

    QCOMPARE(handler.intrinsicSize(&doc, 0, format), expectedSize);
}
#endif

QTEST_MAIN(tst_QTextImageHandler)
#include "tst_qtextimagehandler.moc"
