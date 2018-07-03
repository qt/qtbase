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
    // todo: glBaseInternalFormat
    QTest::addColumn<int>("levels");
    QTest::addColumn<int>("dataOffset");
    QTest::addColumn<int>("dataLength");

    QTest::addRow("pattern.pkm") << QStringLiteral(":/texturefiles/pattern.pkm")
                                 << QSize(64, 64)
                                 << quint32(0x0)
                                 << quint32(0x8d64)
                                 << 1
                                 << 16
                                 << 2048;

    QTest::addRow("car.ktx") << QStringLiteral(":/texturefiles/car.ktx")
                             << QSize(146, 80)
                             << quint32(0x0)
                             << quint32(0x9278)
                             << 1
                             << 68
                             << 11840;
}

void tst_qtexturefilereader::checkHandlers()
{
    QFETCH(QString, fileName);
    QFETCH(QSize, size);
    QFETCH(quint32, glFormat);
    QFETCH(quint32, glInternalFormat);
    QFETCH(int, levels);
    QFETCH(int, dataOffset);
    QFETCH(int, dataLength);

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
    QCOMPARE(tex.dataOffset(), dataOffset);
    QCOMPARE(tex.dataLength(), dataLength);
}

QTEST_MAIN(tst_qtexturefilereader)

#include "tst_qtexturefilereader.moc"
