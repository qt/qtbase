/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <private/qtexturefilereader_p.h>
#include <QtTest>

class tst_qtexturefilereader : public QObject
{
    Q_OBJECT

private slots:
    void checkHandlers_data();
    void checkHandlers();
};

void tst_qtexturefilereader::checkHandlers_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QSize>("size");
    QTest::addColumn<quint32>("glFormat");
    QTest::addColumn<quint32>("glInternalFormat");
    QTest::addColumn<quint32>("glBaseInternalFormat");
    QTest::addColumn<int>("levels");
    QTest::addColumn<QList<int>>("dataOffsets");
    QTest::addColumn<QList<int>>("dataLengths");

    QTest::addRow("pattern.pkm")
            << QStringLiteral(":/texturefiles/pattern.pkm")
            << QSize(64, 64)
            << quint32(0x0)
            << quint32(0x8d64)
            << quint32(0x0)
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
            << (QList<int>() << 68)
            << (QList<int>() << 11840);

    QTest::addRow("car_mips.ktx")
            << QStringLiteral(":/texturefiles/car_mips.ktx")
            << QSize(146, 80)
            << quint32(0x0)
            << quint32(0x9274)
            << quint32(0x1907)
            << 8
            << (QList<int>() << 68 << 5992 << 7516 << 7880 << 8004 << 8056 << 8068 << 8080)
            << (QList<int>() << 5920 << 1520 << 360 << 120 << 48 << 8 << 8 << 8);

    QTest::addRow("newlogo.astc")
            << QStringLiteral(":/texturefiles/newlogo.astc")
            << QSize(111, 78)
            << quint32(0x0)
            << quint32(0x93b9)
            << quint32(0x0)
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
    for (int i = 0; i < tex.numLevels(); i++) {
        QCOMPARE(tex.dataOffset(i), dataOffsets.at(i));
        QCOMPARE(tex.dataLength(i), dataLengths.at(i));
    }
}

QTEST_MAIN(tst_qtexturefilereader)

#include "tst_qtexturefilereader.moc"
