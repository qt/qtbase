/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
    qSetGlobalQHashSeed(0);   // Avoid rendering variations caused by QHash randomization

    QBaselineTest::handleCmdLineArgs(&argc, &argv);
    return _realmain(argc, argv);
}

#include "tst_baseline_text.moc"
