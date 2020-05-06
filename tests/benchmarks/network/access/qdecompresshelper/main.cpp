/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
        QVERIFY(helper.isValid());

        helper.feed(file.readAll());

        qsizetype bytes = 0;
        while (helper.hasData()) {
            QByteArray out(64 * 1024, Qt::Uninitialized);
            qsizetype bytesRead = helper.read(out.data(), out.size());
            bytes += bytesRead;
        }

        QCOMPARE(bytes, 50 * 1024 * 1024);
    }
}

QTEST_MAIN(tst_QDecompressHelper)

#include "main.moc"
