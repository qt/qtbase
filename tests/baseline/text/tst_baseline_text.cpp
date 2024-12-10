// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qbaselinetest.h>
#include <qwidgetbaselinetest.h>
#include <QtWidgets>

class tst_Text : public QWidgetBaselineTest
{
    Q_OBJECT

public:
    tst_Text();

    void loadTestFiles();

private slots:
    void tst_render_data();
    void tst_render();
    void tst_differentScriptsBackgrounds();
    void tst_synthesizedObliqueAndRotation();
    void tst_disableEmojiParsing_data();
    void tst_disableEmojiParsing();

private:
    QDir htmlDir;
};

tst_Text::tst_Text()
{
    QString baseDir = QFINDTESTDATA("data/empty.html");
    htmlDir = QDir(QFileInfo(baseDir).path());
}

void tst_Text::loadTestFiles()
{
    QTest::addColumn<QString>("html");

    QStringList htmlFiles;
    // first add generic test files
    for (const auto &qssFile : htmlDir.entryList({QStringLiteral("*.html")}, QDir::Files | QDir::Readable))
        htmlFiles << htmlDir.absoluteFilePath(qssFile);

    // then test-function specific files
    const QString testFunction = QString(QTest::currentTestFunction()).remove("tst_").toLower();
    if (htmlDir.cd(testFunction)) {
        for (const auto &htmlFile : htmlDir.entryList({QStringLiteral("*.html")}, QDir::Files | QDir::Readable))
            htmlFiles << htmlDir.absoluteFilePath(htmlFile);
        htmlDir.cdUp();
    }

    for (const auto &htmlFile : htmlFiles) {
        QFileInfo fileInfo(htmlFile);
        QFile file(htmlFile);
        QVERIFY(file.open(QFile::ReadOnly));
        QString html = QString::fromUtf8(file.readAll());
        QBaselineTest::newRow(fileInfo.baseName().toUtf8()) << html;
    }
}

void tst_Text::tst_render_data()
{
    loadTestFiles();
}

void tst_Text::tst_render()
{
    QFETCH(QString, html);

    QTextDocument textDocument;
    textDocument.setPageSize(QSizeF(800, 600));
    textDocument.setHtml(html);

    QImage image(800, 600, QImage::Format_ARGB32);
    image.fill(Qt::white);

    {
        QPainter painter(&image);

        QAbstractTextDocumentLayout::PaintContext context;
        context.palette.setColor(QPalette::Text, Qt::black);
        textDocument.documentLayout()->draw(&painter, context);
    }

    QBASELINE_TEST(image);
}

void tst_Text::tst_differentScriptsBackgrounds()
{
    QTextDocument textDocument;
    textDocument.setPageSize(QSizeF(800, 600));
    textDocument.setHtml(QString::fromUtf8("<i><font style=\"font-size:72px\"><font style=\"background:#FFFF00\">イ雨ｴ</font></font></i>"));

    QImage image(800, 600, QImage::Format_ARGB32);
    image.fill(Qt::white);

    {
        QPainter painter(&image);

        QAbstractTextDocumentLayout::PaintContext context;
        context.palette.setColor(QPalette::Text, Qt::black);
        textDocument.documentLayout()->draw(&painter, context);
    }

    QBASELINE_CHECK(image, "tst_differentScriptsBackgrounds");
}

void tst_Text::tst_synthesizedObliqueAndRotation()
{
    QFont font(QString::fromLatin1("Abyssinica SIL"));
    font.setPixelSize(40);
    font.setItalic(true);

    QImage image(800, 600, QImage::Format_ARGB32);
    image.fill(Qt::white);

    {
        QPainter painter(&image);
        painter.setFont(font);

        painter.save();
        painter.translate(200, 450);
        painter.rotate(270);
        painter.drawText(0, 0, QString::fromLatin1("Foobar"));
        painter.restore();
    }

    QBASELINE_CHECK(image, "tst_synthesizedObliqueAndRotation");
}

void tst_Text::tst_disableEmojiParsing_data()
{
    QTest::addColumn<QString>("html");

    QStringList htmlFiles;
    // first add generic test files
    for (const auto &qssFile : htmlDir.entryList({QStringLiteral("emoji*.html")}, QDir::Files | QDir::Readable))
        htmlFiles << htmlDir.absoluteFilePath(qssFile);

    // then test-function specific files
    const QString testFunction = QString(QTest::currentTestFunction()).remove("tst_").toLower();
    if (htmlDir.cd(testFunction)) {
        for (const auto &htmlFile : htmlDir.entryList({QStringLiteral("*.html")}, QDir::Files | QDir::Readable))
            htmlFiles << htmlDir.absoluteFilePath(htmlFile);
        htmlDir.cdUp();
    }

    for (const auto &htmlFile : htmlFiles) {
        QFileInfo fileInfo(htmlFile);
        QFile file(htmlFile);
        QVERIFY(file.open(QFile::ReadOnly));
        QString html = QString::fromUtf8(file.readAll());
        QBaselineTest::newRow(fileInfo.baseName().toUtf8()) << html;
    }
}

void tst_Text::tst_disableEmojiParsing()
{
    QFETCH(QString, html);

    QTextDocument textDocument;
    textDocument.setPageSize(QSizeF(800, 600));
    textDocument.setHtml(html);

    QTextOption opt = textDocument.defaultTextOption();
    opt.setFlags(QTextOption::DisableEmojiParsing);
    textDocument.setDefaultTextOption(opt);

    QImage image(800, 600, QImage::Format_ARGB32);
    image.fill(Qt::white);

    {
        QPainter painter(&image);

        QAbstractTextDocumentLayout::PaintContext context;
        context.palette.setColor(QPalette::Text, Qt::black);
        textDocument.documentLayout()->draw(&painter, context);
    }

    QBASELINE_CHECK(image, "tst_disableEmojiParsing");
}

QBASELINETEST_MAIN(tst_Text)

#include "tst_baseline_text.moc"
