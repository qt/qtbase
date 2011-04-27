/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QtTest/QtTest>
#include <QtGui>
#include <QtCore>

#if defined(Q_OS_SYMBIAN)
# define STRINGIFY(x) #x
# define TOSTRING(x) STRINGIFY(x)
# define SRCDIR "C:/Private/" TOSTRING(SYMBIAN_SRCDIR_UID) "/"
#endif

class tst_QIcoImageFormat : public QObject
{
    Q_OBJECT

public:
    tst_QIcoImageFormat();
    virtual ~tst_QIcoImageFormat();


public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
private slots:
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

private:
    QString m_IconPath;
};


tst_QIcoImageFormat::tst_QIcoImageFormat()
{
    m_IconPath = QLatin1String(SRCDIR) + "/icons";
    qDebug() << m_IconPath;
}

tst_QIcoImageFormat::~tst_QIcoImageFormat()
{

}

void tst_QIcoImageFormat::init()
{

}

void tst_QIcoImageFormat::cleanup()
{

}

void tst_QIcoImageFormat::initTestCase()
{

}

void tst_QIcoImageFormat::cleanupTestCase()
{

}

void tst_QIcoImageFormat::format()
{
    QImageReader reader(m_IconPath + "/valid/35FLOPPY.ICO", "ico");
    QByteArray fmt = reader.format();
    QCOMPARE(const_cast<const char*>(fmt.data()), "ico" );
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
    QTest::newRow("16px16c, 32px32c, 32px256c") << "valid/WORLD.ico" << 1;
    QTest::newRow("16px16c, 32px32c, 32px256c") << "valid/WORLDH.ico" << 1;
    QTest::newRow("invalid floppy (first 8 bytes = 0xff)") << "invalid/35floppy.ico" << 0;
    QTest::newRow("103x16px, 24BPP") << "valid/trolltechlogo_tiny.ico" << 1;
    QTest::newRow("includes 32BPP w/alpha") << "valid/semitransparent.ico" << 1;
    QTest::newRow("PNG compression") << "valid/Qt.ico" << 1;
}

void tst_QIcoImageFormat::canRead()
{
    QFETCH(QString, fileName);
    QFETCH(int, isValid);

    QImageReader reader(m_IconPath + "/" + fileName);
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

    QSequentialFile *file = new QSequentialFile(m_IconPath + "/" + fileName);
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
    QTest::newRow("16px16c, 32px32c, 32px256c") << "valid/WORLD.ico" << 3;
    QTest::newRow("16px16c, 32px32c, 32px256c") << "valid/WORLDH.ico" << 3;
    QTest::newRow("invalid floppy (first 8 bytes = 0xff)") << "invalid/35floppy.ico" << 0;
    QTest::newRow("includes 32BPP w/alpha") << "valid/semitransparent.ico" << 9;
    QTest::newRow("PNG compression") << "valid/Qt.ico" << 4;

}

void tst_QIcoImageFormat::imageCount()
{
    QFETCH(QString, fileName);
    QFETCH(int, count);

    QImageReader reader(m_IconPath + "/" + fileName);
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
    QTest::newRow("16px16c, 32px32c, 32px256c") << "valid/WORLD.ico" << 3;
    QTest::newRow("16px16c, 32px32c, 32px256c") << "valid/WORLDH.ico" << 3;
    QTest::newRow("includes 32BPP w/alpha") << "valid/semitransparent.ico" << 9;
    QTest::newRow("PNG compression") << "valid/Qt.ico" << 4;
}

void tst_QIcoImageFormat::jumpToNextImage()
{
    QFETCH(QString, fileName);
    QFETCH(int, count);

    QImageReader reader(m_IconPath + "/" + fileName);
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
    QTest::newRow("invalid floppy (first 8 bytes = 0xff)") << "invalid/35floppy.ico" << 0;
}

void tst_QIcoImageFormat::loopCount()
{
    QFETCH(QString, fileName);
    QFETCH(int, count);

    QImageReader reader(m_IconPath + "/" + fileName);
    QCOMPARE(reader.loopCount(), count);
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
    QTest::newRow("16px16c, 32px32c, 32px256c") << "valid/WORLD.ico" << 3;
    QTest::newRow("16px16c, 32px32c, 32px256c") << "valid/WORLDH.ico" << 3;
    QTest::newRow("invalid floppy (first 8 bytes = 0xff)") << "invalid/35floppy.ico" << -1;
    QTest::newRow("includes 32BPP w/alpha") << "valid/semitransparent.ico" << 9;
    QTest::newRow("PNG compression") << "valid/Qt.ico" << 4;
}

void tst_QIcoImageFormat::nextImageDelay()
{
    QFETCH(QString, fileName);
    QFETCH(int, count);

    QImageReader reader(m_IconPath + "/" + fileName);
    if (count == -1) {
        QCOMPARE(reader.nextImageDelay(), 0);
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

    QImageReader reader(m_IconPath + "/" + fileName);

    QImage image;
    reader.jumpToImage(index);
    reader.read(&image);

    QCOMPARE(image.width(), width);
    QCOMPARE(image.height(), height);
}

QTEST_MAIN(tst_QIcoImageFormat)
#include "tst_qicoimageformat.moc"

