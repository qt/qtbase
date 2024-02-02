// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2021 Igor Kushnir <igorkuo@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QMimeDatabase>

using namespace Qt::StringLiterals;

namespace {
struct MatchModeInfo
{
    QMimeDatabase::MatchMode mode;
    const char *name;
};

constexpr MatchModeInfo matchModes[] = { { QMimeDatabase::MatchDefault, "Default" },
                                         { QMimeDatabase::MatchExtension, "Extension" },
                                         { QMimeDatabase::MatchContent, "Content" } };

void addFileRows(const char *tag, const QString &fileName, const QStringList &expectedMimeNames)
{
    QCOMPARE(static_cast<std::size_t>(expectedMimeNames.size()), std::size(matchModes));
    for (int i = 0; i < expectedMimeNames.size(); ++i) {
        QTest::addRow(qPrintable(tag + QStringLiteral(" - %s")), matchModes[i].name)
                << fileName << matchModes[i].mode << expectedMimeNames[i];
    }
}

void addExistentFileRows(const char *tag, const QString &fileName,
                         const QStringList &expectedMimeNames)
{
    const QString filePath = QFINDTESTDATA("files/" + fileName);
    QVERIFY2(!filePath.isEmpty(),
             qPrintable(QStringLiteral("Cannot find test file %1 in files/").arg(fileName)));
    addFileRows(tag, filePath, expectedMimeNames);
}
}

class tst_QMimeDatabase: public QObject
{

    Q_OBJECT

private slots:
    void inheritsPerformance();
    void benchMimeTypeForName();
    void benchMimeTypeForFile_data();
    void benchMimeTypeForFile();
};

void tst_QMimeDatabase::inheritsPerformance()
{
    // Check performance of inherits().
    // This benchmark (which started in 2009 in kmimetypetest.cpp) uses 40 mimetypes.
    // (eight groups of five unique ones)
    const QString uniqueMimeTypes[] = {
        u"image/jpeg"_s,
        u"image/png"_s,
        u"image/tiff"_s,
        u"text/plain"_s,
        u"text/html"_s,
    };
    constexpr size_t NumOuterLoops = 40 / std::size(uniqueMimeTypes);
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForName(u"text/x-chdr"_s);
    QVERIFY(mime.isValid());
    QString match;
    QBENCHMARK {
        for (size_t i = 0; i < NumOuterLoops; ++i) {
            for (const QString &mt : uniqueMimeTypes) {
                if (mime.inherits(mt)) {
                    match = mt;
                    // of course there would normally be a "break" here, but
                    // we're testing worse-case performance here
                }
            }
        }
    }
    QCOMPARE(match, u"text/plain"_s);
    // Numbers from 2011, in release mode:
    // KDE 4.7 numbers: 0.21 msec / 494,000 ticks / 568,345 instr. loads per iteration
    // QMimeBinaryProvider (with Qt 5): 0.16 msec / NA / 416,049 instr. reads per iteration
    // QMimeXmlProvider (with Qt 5): 0.062 msec / NA / 172,889 instr. reads per iteration
    //   (but the startup time is way higher)
    // And memory usage is flat at 200K with QMimeBinaryProvider, while it peaks at 6 MB when
    // parsing XML, and then keeps being around 4.5 MB for all the in-memory hashes.
}

void tst_QMimeDatabase::benchMimeTypeForName()
{
    QMimeDatabase db;

    QBENCHMARK {
        const auto s = db.mimeTypeForName(QStringLiteral("text/plain"));
        QVERIFY(s.isValid());
    }
}

void tst_QMimeDatabase::benchMimeTypeForFile_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QMimeDatabase::MatchMode>("mode");
    QTest::addColumn<QString>("expectedMimeName");

    addFileRows("archive", "a.tar.gz",
                { "application/x-compressed-tar",
                  "application/x-compressed-tar",
                  "application/octet-stream" });
    addFileRows("OpenDocument Text", "b.odt",
                { "application/vnd.oasis.opendocument.text",
                  "application/vnd.oasis.opendocument.text",
                  "application/octet-stream" });

    addExistentFileRows(
            "existent archive with extension", "N.tar.gz",
            { "application/x-compressed-tar", "application/x-compressed-tar", "application/gzip" });
    addExistentFileRows("existent C with extension", "t.c",
                        { "text/x-csrc", "text/x-csrc", "text/plain" });
    addExistentFileRows("existent text file with extension", "u.txt",
                        { "text/plain", "text/plain", "text/plain" });
    addExistentFileRows("existent C w/o extension", "X",
                        { "text/x-csrc", "application/octet-stream", "text/x-csrc" });
    addExistentFileRows("existent patch w/o extension", "y",
                        { "text/x-patch", "application/octet-stream", "text/x-patch" });
    addExistentFileRows("existent archive w/o extension", "z",
                        { "application/gzip", "application/octet-stream", "application/gzip" });
}

void tst_QMimeDatabase::benchMimeTypeForFile()
{
    QFETCH(const QString, fileName);
    QFETCH(const QMimeDatabase::MatchMode, mode);
    QFETCH(const QString, expectedMimeName);

    QMimeDatabase db;

    QBENCHMARK {
        const auto mimeType = db.mimeTypeForFile(fileName, mode);
        QCOMPARE(mimeType.name(), expectedMimeName);
    }
}

QTEST_MAIN(tst_QMimeDatabase)

#include "tst_bench_qmimedatabase.moc"
