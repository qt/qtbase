// Copyright (C) 2026 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QTemporaryDir>
#include <QTemporaryFile>

#include <QtCore/private/qfilesystemengine_p.h>
#include <QtCore/private/qfilesystementry_p.h>

class tst_QFileSystemEngine : public QObject
{
    Q_OBJECT

private slots:
    void nativeIdDefaultInvalid();
    void nativeIdToByteArray();
    void nativeIdSameFile();
    void nativeIdDifferentFiles();
    void nativeIdNonExistent();
};

static QFileSystemNativeId idOf(const QString &path)
{
    return QFileSystemEngine::nativeId(QFileSystemEntry(path));
}

void tst_QFileSystemEngine::nativeIdDefaultInvalid()
{
    QFileSystemNativeId id;
    QVERIFY(!id.isValid());
    QVERIFY(id.toByteArray().isNull());
    QCOMPARE(id, QFileSystemNativeId{});
    // A hashable, comparable default is fine; just ensure qHash doesn't crash.
    QCOMPARE(qHash(id), qHash(QFileSystemNativeId{}));
}

void tst_QFileSystemEngine::nativeIdToByteArray()
{
    QTemporaryFile file;
    QVERIFY2(file.open(), qPrintable(file.errorString()));
    const QFileSystemNativeId id = idOf(file.fileName());
    QVERIFY(id.isValid());
    // id() is documented to be the string form of the native id.
    QCOMPARE(QFileSystemEngine::id(QFileSystemEntry(file.fileName())), id.toByteArray());
    QVERIFY(!id.toByteArray().isEmpty());
}

void tst_QFileSystemEngine::nativeIdSameFile()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));

    QFile f(dir.filePath("file"));
    QVERIFY2(f.open(QIODevice::WriteOnly), qPrintable(f.errorString()));
    f.close();

    // The same file reached via two syntactically different paths must have
    // the same native id (and therefore the same hash).
    const QFileSystemNativeId a = idOf(dir.filePath("file"));
    const QFileSystemNativeId b = idOf(dir.filePath("./file"));
    QVERIFY(a.isValid());
    QVERIFY(b.isValid());
    QCOMPARE(a, b);
    QCOMPARE(qHash(a), qHash(b));
    QVERIFY(!a.toByteArray().isEmpty());
    QCOMPARE(a.toByteArray(), b.toByteArray());
}

void tst_QFileSystemEngine::nativeIdDifferentFiles()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));

    for (const char *name : { "one", "two" }) {
        QFile f(dir.filePath(QLatin1String(name)));
        QVERIFY2(f.open(QIODevice::WriteOnly), qPrintable(f.errorString()));
    }

    const QFileSystemNativeId a = idOf(dir.filePath("one"));
    const QFileSystemNativeId b = idOf(dir.filePath("two"));
    QVERIFY(a.isValid());
    QVERIFY(b.isValid());
    QVERIFY(a != b);
    QVERIFY(!a.toByteArray().isEmpty());
    QVERIFY(!b.toByteArray().isEmpty());
    QVERIFY(a.toByteArray() != b.toByteArray());
}

void tst_QFileSystemEngine::nativeIdNonExistent()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));
    const QFileSystemNativeId id = idOf(dir.filePath("does-not-exist"));
    QVERIFY(!id.isValid());
    QVERIFY(id.toByteArray().isNull());
}

QTEST_MAIN(tst_QFileSystemEngine)
#include "tst_qfilesystemengine.moc"
