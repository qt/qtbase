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

#include <qtest.h>
#include <QBuffer>
#include <QDebug>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QPixmap>
#include <QSet>
#include <QTimer>

typedef QMap<QString, QString> QStringMap;
typedef QList<int> QIntList;
Q_DECLARE_METATYPE(QStringMap)
Q_DECLARE_METATYPE(QIntList)

class tst_QImageReader : public QObject
{
    Q_OBJECT

public:
    tst_QImageReader();
    virtual ~tst_QImageReader();

public slots:
    void initTestCase();
    void init();
    void cleanup();

private slots:
    void readImage_data();
    void readImage();

    void setScaledSize_data();
    void setScaledSize();

    void setClipRect_data();
    void setClipRect();

    void setScaledClipRect_data();
    void setScaledClipRect();

private:
    QList< QPair<QString, QByteArray> > images; // filename, format
    QString prefix;
};

tst_QImageReader::tst_QImageReader()
{
    images << QPair<QString, QByteArray>(QLatin1String("colorful.bmp"), QByteArray("bmp"));
    images << QPair<QString, QByteArray>(QLatin1String("font.bmp"), QByteArray("bmp"));
    images << QPair<QString, QByteArray>(QLatin1String("crash-signed-char.bmp"), QByteArray("bmp"));
    images << QPair<QString, QByteArray>(QLatin1String("4bpp-rle.bmp"), QByteArray("bmp"));
    images << QPair<QString, QByteArray>(QLatin1String("tst7.bmp"), QByteArray("bmp"));
    images << QPair<QString, QByteArray>(QLatin1String("16bpp.bmp"), QByteArray("bmp"));
    images << QPair<QString, QByteArray>(QLatin1String("negativeheight.bmp"), QByteArray("bmp"));
    images << QPair<QString, QByteArray>(QLatin1String("marble.xpm"), QByteArray("xpm"));
    images << QPair<QString, QByteArray>(QLatin1String("kollada.png"), QByteArray("png"));
    images << QPair<QString, QByteArray>(QLatin1String("teapot.ppm"), QByteArray("ppm"));
    images << QPair<QString, QByteArray>(QLatin1String("runners.ppm"), QByteArray("ppm"));
    images << QPair<QString, QByteArray>(QLatin1String("test.ppm"), QByteArray("ppm"));
    images << QPair<QString, QByteArray>(QLatin1String("gnus.xbm"), QByteArray("xbm"));
#if defined QTEST_HAVE_JPEG
    images << QPair<QString, QByteArray>(QLatin1String("beavis.jpg"), QByteArray("jpeg"));
    images << QPair<QString, QByteArray>(QLatin1String("YCbCr_cmyk.jpg"), QByteArray("jpeg"));
    images << QPair<QString, QByteArray>(QLatin1String("YCbCr_rgb.jpg"), QByteArray("jpeg"));
    images << QPair<QString, QByteArray>(QLatin1String("task210380.jpg"), QByteArray("jpeg"));
#endif
#if defined QTEST_HAVE_GIF
    images << QPair<QString, QByteArray>(QLatin1String("earth.gif"), QByteArray("gif"));
    images << QPair<QString, QByteArray>(QLatin1String("trolltech.gif"), QByteArray("gif"));
#endif
}

tst_QImageReader::~tst_QImageReader()
{
}

void tst_QImageReader::initTestCase()
{
    prefix = QFINDTESTDATA("images/");
    if (prefix.isEmpty())
        QFAIL("Can't find images directory!");
}

void tst_QImageReader::init()
{
}

void tst_QImageReader::cleanup()
{
}

void tst_QImageReader::readImage_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QByteArray>("format");

    for (int i = 0; i < images.size(); ++i) {
        const QString file = images[i].first;
        const QByteArray format = images[i].second;
        QTest::newRow(qPrintable(file)) << file << format;
    }
}

void tst_QImageReader::readImage()
{
    QFETCH(QString, fileName);
    QFETCH(QByteArray, format);

    QBENCHMARK {
        QImageReader io(prefix + fileName, format);
        QImage image = io.read();
        QVERIFY(!image.isNull());
    }
}

void tst_QImageReader::setScaledSize_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QByteArray>("format");
    QTest::addColumn<QSize>("newSize");

    for (int i = 0; i < images.size(); ++i) {
        const QString file = images[i].first;
        const QByteArray format = images[i].second;
        QSize size(200, 200);
        if (file == QLatin1String("teapot"))
            size = QSize(400, 400);
        else if (file == QLatin1String("test.ppm"))
            size = QSize(10, 10);
        QTest::newRow(qPrintable(file)) << file << format << size;
    }
}

void tst_QImageReader::setScaledSize()
{
    QFETCH(QString, fileName);
    QFETCH(QSize, newSize);
    QFETCH(QByteArray, format);

    QBENCHMARK {
        QImageReader reader(prefix + fileName, format);
        reader.setScaledSize(newSize);
        QImage image = reader.read();
        QCOMPARE(image.size(), newSize);
    }
}

void tst_QImageReader::setClipRect_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QByteArray>("format");
    QTest::addColumn<QRect>("newRect");

    for (int i = 0; i < images.size(); ++i) {
        const QString file = images[i].first;
        const QByteArray format = images[i].second;
        QTest::newRow(qPrintable(file)) << file << format << QRect(0, 0, 50, 50);
    }
}

void tst_QImageReader::setClipRect()
{
    QFETCH(QString, fileName);
    QFETCH(QRect, newRect);
    QFETCH(QByteArray, format);

    QBENCHMARK {
        QImageReader reader(prefix + fileName, format);
        reader.setClipRect(newRect);
        QImage image = reader.read();
        QCOMPARE(image.rect(), newRect);
    }
}

void tst_QImageReader::setScaledClipRect_data()
{
    setClipRect_data();
}

void tst_QImageReader::setScaledClipRect()
{
    QFETCH(QString, fileName);
    QFETCH(QRect, newRect);
    QFETCH(QByteArray, format);

    QBENCHMARK {
        QImageReader reader(prefix + fileName, format);
        reader.setScaledSize(QSize(300, 300));
        reader.setScaledClipRect(newRect);
        QImage image = reader.read();
        QCOMPARE(image.rect(), newRect);
    }
}

QTEST_MAIN(tst_QImageReader)
#include "tst_qimagereader.moc"
