// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <private/qtexturefilereader_p.h>
#include <QTest>

class tst_qtexturefilereader : public QObject
{
    Q_OBJECT

private slots:
    void checkHandlers_data();
    void checkHandlers();
    void checkMetadata();
};

void tst_qtexturefilereader::checkHandlers_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QSize>("size");
    QTest::addColumn<quint32>("glFormat");
    QTest::addColumn<quint32>("glInternalFormat");
    QTest::addColumn<quint32>("glBaseInternalFormat");
    QTest::addColumn<int>("levels");
    QTest::addColumn<int>("faces");
    QTest::addColumn<QList<int>>("dataOffsets");
    QTest::addColumn<QList<int>>("dataLengths");

    QTest::addRow("pattern.pkm")
            << QStringLiteral(":/texturefiles/pattern.pkm")
            << QSize(64, 64)
            << quint32(0x0)
            << quint32(0x8d64)
            << quint32(0x0)
            << 1
            << 1
            << (QList<int>() << 16)
            << (QList<int>() << 2048);

    QTest::addRow("car.ktx")
            << QStringLiteral(":/texturefiles/car.ktx")
            << QSize(146, 80)
            << quint32(0x0)
            << quint32(0x9278)
            << quint32(0x1908)
            << 1
            << 1
            << (QList<int>() << 68)
            << (QList<int>() << 11840);

    QTest::addRow("car_mips.ktx")
            << QStringLiteral(":/texturefiles/car_mips.ktx")
            << QSize(146, 80)
            << quint32(0x0)
            << quint32(0x9274)
            << quint32(0x1907)
            << 8
            << 1
            << (QList<int>() << 68 << 5992 << 7516 << 7880 << 8004 << 8056 << 8068 << 8080)
            << (QList<int>() << 5920 << 1520 << 360 << 120 << 48 << 8 << 8 << 8);

    QTest::addRow("cubemap_float32_rgba.ktx")
            << QStringLiteral(":/texturefiles/cubemap_float32_rgba.ktx")
            << QSize(16, 16)
            << quint32(0x1908)
            << quint32(0x8814)
            << quint32(0x1908)
            << 5
            << 6
            << (QList<int>() << 96 << 24676 << 30824 << 32364 << 32752)
            << (QList<int>() << 4096 << 1024 << 256 << 64 << 16);

    QTest::addRow("newlogo.astc")
            << QStringLiteral(":/texturefiles/newlogo.astc")
            << QSize(111, 78)
            << quint32(0x0)
            << quint32(0x93b9)
            << quint32(0x0)
            << 1
            << 1
            << (QList<int>() << 16)
            << (QList<int>() << 2496);

    QTest::addRow("newlogo_srgb.astc")
            << QStringLiteral(":/texturefiles/newlogo_srgb.astc")
            << QSize(111, 78)
            << quint32(0x0)
            << quint32(0x93d9)
            << quint32(0x0)
            << 1
            << 1
            << (QList<int>() << 16)
            << (QList<int>() << 2496);
}

void tst_qtexturefilereader::checkHandlers()
{
    QFETCH(QString, fileName);
    QFETCH(QSize, size);
    QFETCH(quint32, glFormat);
    QFETCH(quint32, glInternalFormat);
    QFETCH(int, levels);
    QFETCH(int, faces);
    QFETCH(QList<int>, dataOffsets);
    QFETCH(QList<int>, dataLengths);

    QFile f(fileName);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QTextureFileReader r(&f, fileName);
    QVERIFY(r.canRead());

    QTextureFileData tex = r.read();
    QVERIFY(!tex.isNull());
    QVERIFY(tex.isValid());
    QCOMPARE(tex.size(), size);
    QCOMPARE(tex.glFormat(), glFormat);
    QCOMPARE(tex.glInternalFormat(), glInternalFormat);
    QCOMPARE(tex.numLevels(), levels);
    QCOMPARE(tex.numFaces(), faces);

    for (int i = 0; i < tex.numLevels(); i++) {
        QCOMPARE(tex.dataOffset(i), dataOffsets.at(i));
        QCOMPARE(tex.dataLength(i), dataLengths.at(i));
    }
}

void tst_qtexturefilereader::checkMetadata()
{
    QFile f(":/texturefiles/cubemap_metadata.ktx");
    QVERIFY(f.open(QIODevice::ReadOnly));
    QTextureFileReader r(&f);
    QTextureFileData d = r.read();
    auto kvs = d.keyValueMetadata();

    QVERIFY(kvs.contains("test A"));
    QVERIFY(kvs.contains("test B"));
    QVERIFY(kvs.contains("test C"));
    QCOMPARE(kvs.value("test A"), QByteArrayLiteral("1\x0000"));
    QCOMPARE(kvs.value("test B"), QByteArrayLiteral("2\x0000"));
    QCOMPARE(kvs.value("test C"), QByteArrayLiteral("3\x0000"));
}

QTEST_MAIN(tst_qtexturefilereader)

#include "tst_qtexturefilereader.moc"
