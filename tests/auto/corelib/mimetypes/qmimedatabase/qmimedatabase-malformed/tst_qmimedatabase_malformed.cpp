// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mimecacheprobe.h"

#include <QtTest/QTest>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTemporaryDir>
#include <QtCore/QtEndian>

#include <cstring>

using namespace Qt::StringLiterals;

// Crafted cache entries that need to be found by that lookup must use the same
// string, so keep them tied together.
static constexpr char ProbedFileName[] = "probe.png";
static constexpr char ProbedFileMimeTypeName[] = "image/png";
static constexpr char PngSignature[] = "\x89PNG\r\n\x1a\n";
static constexpr char ProbedIconName[] = "application-octet-stream";
static constexpr char ProbedGenericIconName[] = "application-x-generic";

// Byte offsets of the list-offset fields in the mime.cache header; mirrors the
// anonymous Pos*Offset enum in qmimeprovider.cpp.
enum HeaderField {
    PosAliasList = 4,
    PosParentList = 8,
    PosLiteralList = 12,
    PosReverseSuffixTree = 16,
    PosGlobList = 20,
    PosMagicList = 24,
    PosNamespaceList = 28,
    PosIconsList = 32,
    PosGenericIconsList = 36,
};

static constexpr int HeaderSize = PosGenericIconsList + 4;
static constexpr int AllListFields[] = {
    PosAliasList, PosParentList,  PosLiteralList, PosReverseSuffixTree, PosGlobList,
    PosMagicList, PosNamespaceList, PosIconsList, PosGenericIconsList,
};

// Byte offsets of the "first record" field in the header of the two indirect
// lists; mirrors the PosFirst*Offset enum in qmimeprovider.cpp.
enum IndirectListField {
    PosFirstRoot = 4,
    PosFirstMatch = 8,
};

// An offset stored inside a record, pointing far past the end of any cache
// written here, so the first read through it is out of bounds.
static constexpr quint32 PoisonedOffset = 0x7F000000u;

// Stands in for "the offset of this record's mimetype name", which the cache
// builders only know once they have laid the record out.
static constexpr quint32 NameFieldPlaceholder = 0xFFFFFFFFu;

// Record words that are not offsets: a glob record's flags-and-weight, and a
// magic record's accuracy.
static constexpr quint32 GlobFlagsAndWeight = 50;
static constexpr quint32 MagicAccuracy = 50;

// The shared "this list is empty" region that every non-poisoned list offset
// points at: it starts right after the header, and begins with the quint32
// record count that every list header has.
static constexpr int EmptyListHeaderSize = 12;
static constexpr int EmptyListOffset = HeaderSize;
static constexpr int ListCountSize = 4;

// Where a crafted list header goes: right behind the shared "0 entries" region.
static constexpr int CraftedListOffset = EmptyListOffset + EmptyListHeaderSize;

// The helper application that runs each query in a process of its own, built
// next to the test binary.
static constexpr QLatin1StringView ProbeName("mimecacheprobe");

// ---------------------------------------------------------------------------
class tst_QMimeDatabaseMalformed : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void binaryCacheTruncated_data();
    void binaryCacheTruncated();
    void binaryCacheUnvalidatedOffsets_data();
    void binaryCacheUnvalidatedOffsets();
    void binaryCacheAliasMissingNulTerminator();

private:
    struct VictimResult {
        enum RunStatus { StartFailed, TimedOut, Completed };

        RunStatus m_runStatus = StartFailed;
        QProcess::ExitStatus m_exitStatus = QProcess::CrashExit;
        int m_exitCode = -1;
        QString m_output;

        // The child must have run to completion, exited cleanly, and reported
        // exactly the expected result.
        bool succeeded(QStringView expectedResult) const
        {
            return m_runStatus == Completed && m_exitStatus == QProcess::NormalExit && m_exitCode == 0
                    && resultLine() == expectedResult;
        }

        // The child's single "ok=..." line, stripped of its line ending.
        QStringView resultLine() const
        {
            for (QStringView line : QStringView{m_output}.split(u'\n')) {
                line = line.trimmed();
                if (line.startsWith("ok="_L1))
                    return line;
            }
            return {};
        }

        QString diagnostic(QStringView expectedResult) const
        {
            switch (m_runStatus) {
            case StartFailed:
                return u"child process failed to start"_s;
            case TimedOut:
                return u"child process timed out and was killed; output: "_s + m_output;
            case Completed:
                break;
            }
            if (m_exitStatus == QProcess::CrashExit)
                return u"child process crashed; output: "_s + m_output;
            if (m_exitCode != 0)
                return u"child process exited with code "_s + QString::number(m_exitCode)
                        + u"; output: "_s + m_output;
            return u"child process reported '"_s + resultLine() + u"', expected '"_s
                    + expectedResult + u"'; full output: "_s + m_output;
        }
    };

    // Spawns the probe helper, pointed at mimeParentDir via XDG_*.
    VictimResult runCase(const QString &caseName, const QString &mimeParentDir,
                         const QString &dataFile = QString());

    static QString expectedResult(const QString &queryCase);
    static QByteArray emptyCache();
    static void putRecord(QByteArray &cache, int recordOffset, const QList<quint32> &fields);
    static QByteArray directListCache(int posListField, const QList<quint32> &fields);
    static QByteArray indirectListCache(int posListField, int posFirstRecord,
                                        const QList<quint32> &fields);
    static bool writeFile(const QString &path, const QByteArray &data, QString *err);
    static bool makeMimeDir(const QString &parentDir, QString *err);
    static void put16(QByteArray &buf, int offset, quint16 value);
    static void put32(QByteArray &buf, int offset, quint32 value);

    QString m_probePath;
};

// Check the helper once, up front: without it every row would fail with a bare
// "child process failed to start".
void tst_QMimeDatabaseMalformed::initTestCase()
{
    m_probePath = QCoreApplication::applicationDirPath() + u'/' + ProbeName;
    QVERIFY2(QFileInfo(m_probePath).isExecutable(), qPrintable(m_probePath));
}

QString tst_QMimeDatabaseMalformed::expectedResult(const QString &queryCase)
{
    if (queryCase == QueryCase::Name) {
        // parents and aliases are empty: ProbedMimeTypeName is the default
        // mimetype, so it gets no implicit parent from fallbackParent(), and the
        // built-in database declares neither a parent nor an alias for it.
        return "ok="_L1 + QLatin1StringView(ProbedMimeTypeName) + " parents= aliases="_L1
                + " icon="_L1 + QLatin1StringView(ProbedIconName)
                + " genericIcon="_L1 + QLatin1StringView(ProbedGenericIconName);
    }
    return "ok="_L1 + QLatin1StringView(ProbedFileMimeTypeName);
}

// The smallest well-formed cache: a supported version, and a list-offset table
// where every list points at one shared "0 entries" region.
//   [0,4)   version header: major=1, minor=1
//   [4,40)  list-offset table, one quint32 per Pos* field
//   [40,52) the all-zero "0 entries" region
// The other tests start from this and corrupt or cut exactly one thing, so that
// whatever the child does can only be explained by that one change.
QByteArray tst_QMimeDatabaseMalformed::emptyCache()
{
    QByteArray cache(EmptyListOffset + EmptyListHeaderSize, '\0');
    put16(cache, 0, 1); // major
    put16(cache, 2, 1); // minor (must be 1..2)
    for (const int field : AllListFields)
        put32(cache, field, EmptyListOffset);
    return cache;
}

// Grows cache to hold one record of fields at recordOffset, followed by a
// NUL-terminated ProbedMimeTypeName that NameFieldPlaceholder resolves to.
void tst_QMimeDatabaseMalformed::putRecord(QByteArray &cache, int recordOffset,
                                          const QList<quint32> &fields)
{
    const QByteArray name(ProbedMimeTypeName);
    const int nameOffset = recordOffset + int(fields.size()) * 4;
    cache.resize(nameOffset + int(name.size()) + 1, '\0'); // the fill NUL-terminates name
    for (qsizetype i = 0; i < fields.size(); ++i) {
        const quint32 value = fields.at(i);
        put32(cache, recordOffset + int(i) * 4,
              value == NameFieldPlaceholder ? quint32(nameOffset) : value);
    }
    memcpy(cache.data() + nameOffset, name.constData(), name.size());
}

// A cache whose only non-empty list is the one at posListField, holding a single
// record of fields that is entirely inside the file, so the cache loads and the
// lookup really walks the record.
QByteArray tst_QMimeDatabaseMalformed::directListCache(int posListField,
                                                       const QList<quint32> &fields)
{
    QByteArray cache = emptyCache();
    putRecord(cache, CraftedListOffset + ListCountSize, fields);
    put32(cache, posListField, CraftedListOffset);
    put32(cache, CraftedListOffset, 1); // numEntries
    return cache;
}

// The same, for the two lists whose header does not store the records inline but
// points at them with a "first record" field at posFirstRecord.
QByteArray tst_QMimeDatabaseMalformed::indirectListCache(int posListField, int posFirstRecord,
                                                         const QList<quint32> &fields)
{
    const int recordOffset = CraftedListOffset + posFirstRecord + 4;
    QByteArray cache = emptyCache();
    putRecord(cache, recordOffset, fields);
    put32(cache, posListField, CraftedListOffset);
    put32(cache, CraftedListOffset, 1); // numEntries
    put32(cache, CraftedListOffset + posFirstRecord, recordOffset);
    return cache;
}

bool tst_QMimeDatabaseMalformed::writeFile(const QString &path, const QByteArray &data, QString *err)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        if (err)
            *err = "cannot open "_L1 + path + " for writing: "_L1 + f.errorString();
        return false;
    }
    if (f.write(data) != data.size()) {
        if (err)
            *err = "cannot write "_L1 + path + ": "_L1 + f.errorString();
        return false;
    }
    return true;
}

bool tst_QMimeDatabaseMalformed::makeMimeDir(const QString &parentDir, QString *err)
{
    const QString mimeDir = parentDir + "/mime"_L1;
    if (!QDir().mkpath(mimeDir)) {
        if (err)
            *err = "cannot create "_L1 + mimeDir;
        return false;
    }
    return true;
}

void tst_QMimeDatabaseMalformed::put16(QByteArray &buf, int offset, quint16 value)
{
    Q_ASSERT(offset >= 0 && offset + int(sizeof(value)) <= buf.size());
    qToBigEndian(value, buf.data() + offset);
}

void tst_QMimeDatabaseMalformed::put32(QByteArray &buf, int offset, quint32 value)
{
    Q_ASSERT(offset >= 0 && offset + int(sizeof(value)) <= buf.size());
    qToBigEndian(value, buf.data() + offset);
}

tst_QMimeDatabaseMalformed::VictimResult
tst_QMimeDatabaseMalformed::runCase(const QString &caseName, const QString &mimeParentDir,
                                    const QString &dataFile)
{
    VictimResult r;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    // Restrict discovery to our crafted directory only (plus Qt's built-in DB).
    // QT_NO_MIME_CACHE is deliberately left unset: every case here needs
    // loadProviders() to pick the binary provider for our crafted mime.cache.
    env.remove("QT_NO_MIME_CACHE"_L1);
    env.insert("XDG_DATA_HOME"_L1, mimeParentDir);
    env.insert("XDG_DATA_DIRS"_L1, mimeParentDir);

    QStringList args = { caseName };
    if (!dataFile.isEmpty())
        args << dataFile;

    QProcess proc;
    proc.setProcessEnvironment(env);
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(m_probePath, args);
    if (!proc.waitForStarted(30000))
        return r; // runStatus stays StartFailed
    if (!proc.waitForFinished(120000)) {
        proc.kill();
        proc.waitForFinished();
        r.m_runStatus = VictimResult::TimedOut;
        r.m_output = QString::fromLocal8Bit(proc.readAll());
        return r;
    }
    r.m_runStatus = VictimResult::Completed;
    r.m_exitStatus = proc.exitStatus();
    r.m_exitCode = proc.exitCode();
    r.m_output = QString::fromLocal8Bit(proc.readAll());
    return r;
}

// Every prefix of a well-formed cache is something a consumer can be handed: a
// truncated download, a file being rewritten by update-mime-database while we
// read it, or plain corruption. Each row is the number of bytes kept.
void tst_QMimeDatabaseMalformed::binaryCacheTruncated_data()
{
    QTest::addColumn<int>("keptBytes");

    QTest::newRow("empty") << 0;
    // The major version number only
    QTest::newRow("half-a-version-header") << 2;
    // A complete version number
    QTest::newRow("version-header-only") << 4;
    // The offset table without its last field, PosGenericIconsList = 36.
    QTest::newRow("offset-table-missing-last-field") << int(PosGenericIconsList);
    // A complete header, with nothing where the offsets point.
    QTest::newRow("header-only") << int(HeaderSize);
    // A list's record count is readable, but nothing behind it is.
    QTest::newRow("list-count-only") << int(EmptyListOffset + ListCountSize);
}

// A cache that is too short must be rejected as invalid, without reading past
// the end of the mapping. QMimeDatabase then falls back to another provider,
// so the child still resolves the probed name and exits cleanly.
void tst_QMimeDatabaseMalformed::binaryCacheTruncated()
{
    QFETCH(const int, keptBytes);

    QTemporaryDir tmp;
    QVERIFY2(tmp.isValid(), qPrintable(tmp.errorString()));
    QString err;
    QVERIFY2(makeMimeDir(tmp.path(), &err), qPrintable(err));

    QByteArray cache = emptyCache();
    cache.truncate(keptBytes);
    QVERIFY2(writeFile(tmp.path() + "/mime/mime.cache"_L1, cache, &err), qPrintable(err));

    // The cache is rejected while it is being loaded, before any lookup happens,
    // so the query kind doesn't matter here; "name" is the one needing no file.
    const QString expected = expectedResult(QueryCase::Name);
    const VictimResult r = runCase(QueryCase::Name, tmp.path());
    QVERIFY2(r.succeeded(expected), qPrintable(r.diagnostic(expected)));
}

// Each row gives one list a single in-bounds record holding an out-of-bounds
// offset, which is the part of the format load() cannot check, and runs the query
// whose lookup path walks that list.
void tst_QMimeDatabaseMalformed::binaryCacheUnvalidatedOffsets_data()
{
    QTest::addColumn<QByteArray>("cache");
    QTest::addColumn<QString>("queryCase");

    // Alias, parent and icon records are found by a binary search on the
    // mimetype name they start with, so that name has to be readable for the
    // search to ever reach the poisoned offset behind it.
    QTest::newRow("aliasList")
            << directListCache(PosAliasList, {NameFieldPlaceholder, PoisonedOffset})
            << QString(QueryCase::Name);
    QTest::newRow("parentList")
            << directListCache(PosParentList, {NameFieldPlaceholder, PoisonedOffset})
            << QString(QueryCase::Name);
    QTest::newRow("iconsList")
            << directListCache(PosIconsList, {NameFieldPlaceholder, PoisonedOffset})
            << QString(QueryCase::Name);
    QTest::newRow("genericIconsList")
            << directListCache(PosGenericIconsList, {NameFieldPlaceholder, PoisonedOffset})
            << QString(QueryCase::Name);

    // Every glob record is walked, so here the poison can be the pattern offset.
    QTest::newRow("literalList")
            << directListCache(PosLiteralList,
                               {PoisonedOffset, NameFieldPlaceholder, GlobFlagsAndWeight})
            << QString(QueryCase::FileName);
    QTest::newRow("globList")
            << directListCache(PosGlobList,
                               {PoisonedOffset, NameFieldPlaceholder, GlobFlagsAndWeight})
            << QString(QueryCase::FileName);

    // The suffix tree is walked from the last character of the file name
    // backwards, so the root node has to carry that character for the walk to
    // descend to the poisoned children offset.
    const QString lowerProbedFileName = QLatin1StringView(ProbedFileName).toString().toLower();
    const quint32 lastChar = lowerProbedFileName.back().unicode();
    QTest::newRow("reverseSuffixTree")
            << indirectListCache(PosReverseSuffixTree, PosFirstRoot,
                                 { lastChar, 1 /*numChildren*/, PoisonedOffset })
            << QString(QueryCase::FileName);

    QTest::newRow("magicList")
            << indirectListCache(PosMagicList, PosFirstMatch,
                                 {MagicAccuracy, NameFieldPlaceholder, 1 /*numMatchlets*/,
                                  PoisonedOffset})
            << QString(QueryCase::Content);
}

// A mime.cache that loads cleanly but whose one record points out of the mapping
// must have that record skipped rather than crash, leaving the intact providers
// to produce the exact expected line.
void tst_QMimeDatabaseMalformed::binaryCacheUnvalidatedOffsets()
{
    QFETCH(const QByteArray, cache);
    QFETCH(const QString, queryCase);

    QTemporaryDir tmp;
    QVERIFY2(tmp.isValid(), qPrintable(tmp.errorString()));
    QString err;
    QVERIFY2(makeMimeDir(tmp.path(), &err), qPrintable(err));

    QVERIFY2(writeFile(tmp.path() + "/mime/mime.cache"_L1, cache, &err), qPrintable(err));

    QString dataFile;
    if (queryCase != QueryCase::Name) {
        // A non-empty file the built-in database recognizes both by name and by
        // content, so that either query kind has a right answer to reach.
        dataFile = tmp.path() + u'/' + QLatin1StringView(ProbedFileName);
        QVERIFY2(writeFile(dataFile, QByteArray(PngSignature, sizeof(PngSignature) - 1), &err),
                 qPrintable(err));
    }

    const QString expected = expectedResult(queryCase);
    const VictimResult r = runCase(queryCase, tmp.path(), dataFile);
    QVERIFY2(r.succeeded(expected), qPrintable(r.diagnostic(expected)));
}

// A mime.cache whose single alias-list entry is otherwise well-formed and fully
// in bounds, but whose target-mimetype string has no NUL terminator before the
// end of the file. getLatin1String() must stop scanning at the mapping's end and
// return a null view instead of reading (or crashing) past it. Exercises both
// resolveAlias() (which compares the in-bounds alias name, then reads the
// unterminated target) and addAliases().
void tst_QMimeDatabaseMalformed::binaryCacheAliasMissingNulTerminator()
{
    QTemporaryDir tmp;
    QVERIFY2(tmp.isValid(), qPrintable(tmp.errorString()));
    QString err;
    QVERIFY2(makeMimeDir(tmp.path(), &err), qPrintable(err));

    // Layout (all offsets absolute, in bytes):
    //   [0,4)      version header: major=1, minor=1
    //   [4,40)     list-offset table, one quint32 per Pos*Offset field
    //   [40,52)    shared "0 entries" region for every list but the alias list
    //   [52,56)    alias list: numEntries = 1
    //   [56,64)    alias list entry 0: aliasOffset, then mimeOffset
    //   [64,89)    "application/octet-stream", NUL-terminated by the zero fill
    //   [89,97)    target mimetype bytes, deliberately left without a NUL
    const QByteArray aliasName(ProbedMimeTypeName);
    constexpr int aliasListOffset = EmptyListOffset + EmptyListHeaderSize;
    constexpr int aliasEntryOffset = aliasListOffset + 4;
    constexpr int aliasStrOffset = aliasEntryOffset + 8;
    const int unterminatedOffset = aliasStrOffset + int(aliasName.size()) + 1;
    constexpr int unterminatedSize = 8;
    const int cacheSize = unterminatedOffset + unterminatedSize;

    QByteArray cache = emptyCache();
    cache.resize(cacheSize, '\0');
    put32(cache, PosAliasList, aliasListOffset); // ... except the alias list
    put32(cache, aliasListOffset, 1); // numEntries
    put32(cache, aliasEntryOffset, aliasStrOffset);
    put32(cache, aliasEntryOffset + 4, unterminatedOffset);
    // The alias name's terminator comes from the zero fill; the target string
    // must overwrite its zeros so that no NUL remains before EOF.
    memcpy(cache.data() + aliasStrOffset, aliasName.constData(), aliasName.size());
    memset(cache.data() + unterminatedOffset, 'A', unterminatedSize);
    QVERIFY2(writeFile(tmp.path() + "/mime/mime.cache"_L1, cache, &err), qPrintable(err));

    // The alias must resolve to nothing, leaving mimeTypeForName() to fall back
    // to the probed name itself. If the unterminated string were read as-is,
    // resolveAlias() would return the garbage that follows it and the lookup
    // would end up with an invalid, unnamed QMimeType instead.
    const QString expected = expectedResult(QueryCase::Name);
    const VictimResult r = runCase(QueryCase::Name, tmp.path());
    QVERIFY2(r.succeeded(expected), qPrintable(r.diagnostic(expected)));
}

QTEST_GUILESS_MAIN(tst_QMimeDatabaseMalformed)

#include "tst_qmimedatabase_malformed.moc"
