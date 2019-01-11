/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#include <QtTest/QtTest>
#include <QtGui>
#include <QtCore>

class tst_QIcoImageFormat : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void format();
    void canRead_data();
    void canRead();
    void SequentialFile_data();
    void SequentialFile();
    void imageCount_data();
    void imageCount();
    void jumpToNextImage_data();
    void jumpToNextImage();
    void loopCount_data();
    void loopCount();
    void nextImageDelay_data();
    void nextImageDelay();
    void pngCompression_data();
    void pngCompression();
    void write_data();
    void write();

private:
    QString m_IconPath;
};

void tst_QIcoImageFormat::initTestCase()
{
    m_IconPath = QFINDTESTDATA("icons");
    if (m_IconPath.isEmpty())
        QFAIL("Cannot find icons directory containing testdata!");
}

void tst_QIcoImageFormat::format()
{
    {
        QImageReader reader(m_IconPath + "/valid/35FLOPPY.ICO", "ico");
        QByteArray fmt = reader.format();
        QCOMPARE(const_cast<const char*>(fmt.data()), "ico" );
    }

    {
        QImageReader reader(m_IconPath + "/valid/yellow.cur", "ico");
        QByteArray fmt = reader.format();
        QCOMPARE(const_cast<const char*>(fmt.data()), "ico" );
    }
}

void tst_QIcoImageFormat::canRead_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<int>("isValid");

    QTest::newRow("floppy (16px,32px - 16 colors)") << "valid/35FLOPPY.ICO" << 1;
    QTest::newRow("16px,32px,48px - 256,16M colors") << "valid/abcardWindow.ico" << 1;
    QTest::newRow("16px - 16 colors") << "valid/App.ico" << 1;
    QTest::newRow("16px,32px,48px - 16,256,16M colors") << "valid/Obj_N2_Internal_Mem.ico" << 1;
    QTest::newRow("16px - 16,256,16M colors") << "valid/Status_Play.ico" << 1;
    QTest::newRow("16px,32px - 16 colors") << "valid/TIMER01.ICO" << 1;
    QTest::newRow("16px16c, 32px32c, 32px256c 1") << "valid/WORLD.ico" << 1;
    QTest::newRow("16px16c, 32px32c, 32px256c 2") << "valid/WORLDH.ico" << 1;
    QTest::newRow("invalid floppy (first 8 bytes = 0xff)") << "invalid/35floppy.ico" << 0;
    QTest::newRow("103x16px, 24BPP") << "valid/trolltechlogo_tiny.ico" << 1;
    QTest::newRow("includes 32BPP w/alpha") << "valid/semitransparent.ico" << 1;
    QTest::newRow("PNG compression") << "valid/Qt.ico" << 1;
    QTest::newRow("CUR file") << "valid/yellow.cur" << 1;
}

void tst_QIcoImageFormat::canRead()
{
    QFETCH(QString, fileName);
    QFETCH(int, isValid);

    QImageReader reader(m_IconPath + QLatin1Char('/') + fileName);
    QCOMPARE(reader.canRead(), (isValid == 0 ? false : true));
}

class QSequentialFile : public QFile
{
public:
    QSequentialFile(const QString &name) : QFile(name) {}

    virtual ~QSequentialFile() {}

    virtual bool isSequential() const {
        return true;
    }

};

void tst_QIcoImageFormat::SequentialFile_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<int>("isValid");

    QTest::newRow("floppy (16,32 pixels - 16 colors)") << "valid/35FLOPPY.ICO" << 1;

    QTest::newRow("invalid floppy (first 8 bytes = 0xff)") << "invalid/35floppy.ico" << 0;


}

void tst_QIcoImageFormat::SequentialFile()
{
    QFETCH(QString, fileName);
    QFETCH(int, isValid);

    QSequentialFile *file = new QSequentialFile(m_IconPath + QLatin1Char('/') + fileName);
    QVERIFY(file);
    QVERIFY(file->open(QFile::ReadOnly));
    QImageReader reader(file);

    // Perform the check twice. If canRead() does not restore the sequential device back to its original state,
    // it will fail on the second try.
    QCOMPARE(reader.canRead(), (isValid == 0 ? false : true));
    QCOMPARE(reader.canRead(), (isValid == 0 ? false : true));
    file->close();
}


void tst_QIcoImageFormat::imageCount_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<int>("count");

    QTest::newRow("floppy (16px,32px - 16 colors)") << "valid/35FLOPPY.ICO" << 2;
    QTest::newRow("16px,32px,48px - 256,16M colors") << "valid/abcardWindow.ico" << 6;
    QTest::newRow("16px - 16 colors") << "valid/App.ico" << 1;
    QTest::newRow("16px,32px,48px - 16,256,16M colors") << "valid/Obj_N2_Internal_Mem.ico" << 9;
    QTest::newRow("16px - 16,256,16M colors") << "valid/Status_Play.ico" << 3;
    QTest::newRow("16px,32px - 16 colors") << "valid/TIMER01.ICO" << 2;
    QTest::newRow("16px16c, 32px32c, 32px256c 1") << "valid/WORLD.ico" << 3;
    QTest::newRow("16px16c, 32px32c, 32px256c 2") << "valid/WORLDH.ico" << 3;
    QTest::newRow("invalid floppy (first 8 bytes = 0xff)") << "invalid/35floppy.ico" << -1;
    QTest::newRow("includes 32BPP w/alpha") << "valid/semitransparent.ico" << 9;
    QTest::newRow("PNG compression") << "valid/Qt.ico" << 4;
    QTest::newRow("CUR file") << "valid/yellow.cur" << 1;
}

void tst_QIcoImageFormat::imageCount()
{
    QFETCH(QString, fileName);
    QFETCH(int, count);

    QImageReader reader(m_IconPath + QLatin1Char('/') + fileName);
    QCOMPARE(reader.imageCount(), count);
}

void tst_QIcoImageFormat::jumpToNextImage_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<int>("count");

    QTest::newRow("floppy (16px,32px - 16 colors)") << "valid/35FLOPPY.ICO" << 2;
    QTest::newRow("16px,32px,48px - 256,16M colors") << "valid/abcardWindow.ico" << 6;
    QTest::newRow("16px - 16 colors") << "valid/App.ico" << 1;
    QTest::newRow("16px,32px,48px - 16,256,16M colors") << "valid/Obj_N2_Internal_Mem.ico" << 9;
    QTest::newRow("16px - 16,256,16M colors") << "valid/Status_Play.ico" << 3;
    QTest::newRow("16px,32px - 16 colors") << "valid/TIMER01.ICO" << 2;
    QTest::newRow("16px16c, 32px32c, 32px256c 1") << "valid/WORLD.ico" << 3;
    QTest::newRow("16px16c, 32px32c, 32px256c 2") << "valid/WORLDH.ico" << 3;
    QTest::newRow("includes 32BPP w/alpha") << "valid/semitransparent.ico" << 9;
    QTest::newRow("PNG compression") << "valid/Qt.ico" << 4;
    QTest::newRow("CUR file") << "valid/yellow.cur" << 1;
}

void tst_QIcoImageFormat::jumpToNextImage()
{
    QFETCH(QString, fileName);
    QFETCH(int, count);

    QImageReader reader(m_IconPath + QLatin1Char('/') + fileName);
    bool bJumped = reader.jumpToImage(0);
    while (bJumped) {
        count--;
        bJumped = reader.jumpToNextImage();
    }
    QCOMPARE(count, 0);
}

void tst_QIcoImageFormat::loopCount_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<int>("count");

    QTest::newRow("floppy (16px,32px - 16 colors)") << "valid/35FLOPPY.ICO" << 0;
    QTest::newRow("invalid floppy (first 8 bytes = 0xff)") << "invalid/35floppy.ico" << -1;
}

void tst_QIcoImageFormat::loopCount()
{
    QFETCH(QString, fileName);
    QFETCH(int, count);

    QImageReader reader(m_IconPath + QLatin1Char('/') + fileName);
    QCOMPARE(reader.loopCount(), count);
    QCOMPARE(reader.canRead(), count < 0 ? false : true);
}

void tst_QIcoImageFormat::nextImageDelay_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<int>("count");

    QTest::newRow("floppy (16px,32px - 16 colors)") << "valid/35FLOPPY.ICO" << 2;
    QTest::newRow("16px,32px,48px - 256,16M colors") << "valid/abcardWindow.ico" << 6;
    QTest::newRow("16px - 16 colors") << "valid/App.ico" << 1;
    QTest::newRow("16px,32px,48px - 16,256,16M colors") << "valid/Obj_N2_Internal_Mem.ico" << 9;
    QTest::newRow("16px - 16,256,16M colors") << "valid/Status_Play.ico" << 3;
    QTest::newRow("16px,32px - 16 colors") << "valid/TIMER01.ICO" << 2;
    QTest::newRow("16px16c, 32px32c, 32px256c 1") << "valid/WORLD.ico" << 3;
    QTest::newRow("16px16c, 32px32c, 32px256c 2") << "valid/WORLDH.ico" << 3;
    QTest::newRow("invalid floppy (first 8 bytes = 0xff)") << "invalid/35floppy.ico" << -1;
    QTest::newRow("includes 32BPP w/alpha") << "valid/semitransparent.ico" << 9;
    QTest::newRow("PNG compression") << "valid/Qt.ico" << 4;
    QTest::newRow("CUR file") << "valid/yellow.cur" << 1;
}

void tst_QIcoImageFormat::nextImageDelay()
{
    QFETCH(QString, fileName);
    QFETCH(int, count);

    QImageReader reader(m_IconPath + QLatin1Char('/') + fileName);
    if (count == -1) {
        QCOMPARE(reader.nextImageDelay(), -1);
    } else {
        int i;
        for (i = 0; i < count; i++) {
            QVERIFY(reader.jumpToImage(i));
            QCOMPARE(reader.nextImageDelay(), 0);
        }
    }
}

void tst_QIcoImageFormat::pngCompression_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<int>("index");
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("height");

    QTest::newRow("PNG compression") << "valid/Qt.ico" << 4 << 256 << 256;
}

void tst_QIcoImageFormat::pngCompression()
{
    QFETCH(QString, fileName);
    QFETCH(int, index);
    QFETCH(int, width);
    QFETCH(int, height);

    QImageReader reader(m_IconPath + QLatin1Char('/') + fileName);

    QImage image;
    reader.jumpToImage(index);

    QSize size = reader.size();
    QCOMPARE(size.width(), width);
    QCOMPARE(size.height(), height);

    reader.read(&image);

    QCOMPARE(image.width(), width);
    QCOMPARE(image.height(), height);
}

void tst_QIcoImageFormat::write_data()
{
    QTest::addColumn<QSize>("inSize");
    QTest::addColumn<QSize>("outSize");

    QTest::newRow("64x64") << QSize(64, 64) << QSize(64, 64);
    QTest::newRow("128x200") << QSize(128, 200) << QSize(128, 200);
    QTest::newRow("256x256") << QSize(256, 256) << QSize(256, 256);
    QTest::newRow("400x400") << QSize(400, 400) << QSize(256, 256);
}

void tst_QIcoImageFormat::write()
{
    QFETCH(QSize, inSize);
    QFETCH(QSize, outSize);

    QImage inImg;
    {
        QImageReader reader(m_IconPath + "/valid/Qt.ico");
        reader.jumpToImage(4);
        reader.setScaledSize(inSize);
        inImg = reader.read();
        QVERIFY(!inImg.isNull());
        QCOMPARE(inImg.size(), inSize);
    }

    QBuffer buf;
    {
        buf.open(QIODevice::WriteOnly);
        QImageWriter writer(&buf, "ico");
        QVERIFY(writer.write(inImg));
        buf.close();
    }
    {
        buf.open(QIODevice::ReadOnly);
        QImageReader reader(&buf);
        QVERIFY(reader.canRead());
        QCOMPARE(reader.format(), QByteArray("ico"));
        QImage outImg = reader.read();
        QVERIFY(!outImg.isNull());
        QCOMPARE(outImg.size(), outSize);
        buf.close();
    }
}

QTEST_MAIN(tst_QIcoImageFormat)
#include "tst_qicoimageformat.moc"

