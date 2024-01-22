// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtNetwork/private/qdecompresshelper_p.h>

#include <QtTest/QTest>

class tst_QDecompressHelper : public QObject
{
    Q_OBJECT
private slots:
    void decompress_data();
    void decompress();
};

void tst_QDecompressHelper::decompress_data()
{
    QTest::addColumn<QByteArray>("encoding");
    QTest::addColumn<QString>("fileName");

    QString srcDir = QStringLiteral(QT_STRINGIFY(SRC_DIR));
    srcDir = QDir::fromNativeSeparators(srcDir);
    if (!srcDir.endsWith("/"))
        srcDir += "/";

    bool dataAdded = false;
#ifndef QT_NO_COMPRESS
    QTest::addRow("gzip") << QByteArray("gzip") << srcDir + QString("50mb.txt.gz");
    dataAdded = true;
#endif
#if QT_CONFIG(brotli)
    QTest::addRow("brotli") << QByteArray("br") << srcDir + QString("50mb.txt.br");
    dataAdded = true;
#endif
#if QT_CONFIG(zstd)
    QTest::addRow("zstandard") << QByteArray("zstd") << srcDir + QString("50mb.txt.zst");
    dataAdded = true;
#endif
    if (!dataAdded)
        QSKIP("There's no decompression support");
}

void tst_QDecompressHelper::decompress()
{
    QFETCH(QByteArray, encoding);
    QFETCH(QString, fileName);

    QFile file { fileName };
    QVERIFY(file.open(QIODevice::ReadOnly));
    QBENCHMARK {
        file.seek(0);
        QDecompressHelper helper;
        helper.setEncoding(encoding);
        helper.setDecompressedSafetyCheckThreshold(-1);
        QVERIFY(helper.isValid());

        helper.feed(file.readAll());

        qsizetype bytes = 0;
        QByteArray out(64 * 1024, Qt::Uninitialized);
        while (helper.hasData()) {
            qsizetype bytesRead = helper.read(out.data(), out.size());
            bytes += bytesRead;
        }

        QCOMPARE(bytes, 50 * 1024 * 1024);
    }
}

QTEST_MAIN(tst_QDecompressHelper)

#include "main.moc"
