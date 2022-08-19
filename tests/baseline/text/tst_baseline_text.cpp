// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
        file.open(QFile::ReadOnly);
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


#define main _realmain
QTEST_MAIN(tst_Text)
#undef main

int main(int argc, char *argv[])
{
    // Avoid rendering variations caused by QHash randomization
    QHashSeed::setDeterministicGlobalSeed();

    QBaselineTest::handleCmdLineArgs(&argc, &argv);
    return _realmain(argc, argv);
}

#include "tst_baseline_text.moc"
