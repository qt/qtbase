// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>

#include <QBuffer>
#include <QDebug>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QPixmap>

#include <QtCore/private/qfactoryloader_p.h>
#include <QSet>
#include <QTimer>

typedef QMap<QString, QString> QStringMap;
typedef QList<int> QIntList;
Q_DECLARE_METATYPE(QStringMap)
Q_DECLARE_METATYPE(QIntList)

class tst_bench_QImageReader : public QObject
{
    Q_OBJECT

public:
    tst_bench_QImageReader();

public slots:
    void initTestCase();

private slots:
    void rawFactoryLoader_keyMap();
    void rawFactoryLoader_instance();

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
    QFactoryLoader m_loader{QImageIOHandlerFactoryInterface_iid, "/imageformats"};
};

tst_bench_QImageReader::tst_bench_QImageReader()
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

void tst_bench_QImageReader::initTestCase()
{
    prefix = QFINDTESTDATA("images/");
    if (prefix.isEmpty())
        QFAIL("Can't find images directory!");
}

void tst_bench_QImageReader::rawFactoryLoader_keyMap()
{
    if (m_loader.keyMap().isEmpty())
        QSKIP("No image plugins found.");

    QBENCHMARK {
        [[maybe_unused]] auto r = m_loader.keyMap();
    }
}

void tst_bench_QImageReader::rawFactoryLoader_instance()
{
    if (m_loader.keyMap().isEmpty())
        QSKIP("No image plugins found.");

    const auto numInstances = m_loader.keyMap().uniqueKeys().size();

    QBENCHMARK {
        for (int i = 0; i < numInstances; ++i)
            delete m_loader.instance(i);
    }
}


void tst_bench_QImageReader::readImage_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QByteArray>("format");

    for (int i = 0; i < images.size(); ++i) {
        const QString file = images[i].first;
        const QByteArray format = images[i].second;
        QTest::newRow(qPrintable(file)) << file << format;
    }
}

void tst_bench_QImageReader::readImage()
{
    QFETCH(QString, fileName);
    QFETCH(QByteArray, format);

    QBENCHMARK {
        QImageReader io(prefix + fileName, format);
        QImage image = io.read();
        QVERIFY(!image.isNull());
    }
}

void tst_bench_QImageReader::setScaledSize_data()
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

void tst_bench_QImageReader::setScaledSize()
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

void tst_bench_QImageReader::setClipRect_data()
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

void tst_bench_QImageReader::setClipRect()
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

void tst_bench_QImageReader::setScaledClipRect_data()
{
    setClipRect_data();
}

void tst_bench_QImageReader::setScaledClipRect()
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

QTEST_MAIN(tst_bench_QImageReader)
#include "tst_bench_qimagereader.moc"
