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


#include <QDebug>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QPainter>
#include <QSet>
#include <QTemporaryDir>

#ifdef Q_OS_UNIX // for geteuid()
# include <sys/types.h>
# include <unistd.h>
#endif

#include <algorithm>

typedef QMap<QString, QString> QStringMap;
typedef QVector<int> QIntList;
Q_DECLARE_METATYPE(QImageWriter::ImageWriterError)
Q_DECLARE_METATYPE(QImage::Format)

class tst_QImageWriter : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();

private slots:
    void getSetCheck();
    void writeImage_data();
    void writeImage();
    void writeImage2_data();
    void writeImage2();
    void supportedFormats();
    void supportedMimeTypes();

    void writeToInvalidDevice();
    void testCanWrite();

    void supportsOption_data();
    void supportsOption();

    void saveWithNoFormat_data();
    void saveWithNoFormat();

    void saveToTemporaryFile();

    void writeEmpty();

private:
    QTemporaryDir m_temporaryDir;
    QString prefix;
    QString writePrefix;
};

// helper to skip an autotest when the given image format is not supported
#define SKIP_IF_UNSUPPORTED(format) do {                                                          \
    if (!QByteArray(format).isEmpty() && !QImageReader::supportedImageFormats().contains(format)) \
        QSKIP('"' + QByteArray(format) + "\" images are not supported");             \
} while (0)

static void initializePadding(QImage *image)
{
    int effectiveBytesPerLine = (image->width() * image->depth() + 7) / 8;
    int paddingBytes = image->bytesPerLine() - effectiveBytesPerLine;
    if (paddingBytes == 0)
        return;
    for (int y = 0; y < image->height(); ++y) {
        memset(image->scanLine(y) + effectiveBytesPerLine, 0, paddingBytes);
    }
}

void tst_QImageWriter::initTestCase()
{
    QVERIFY(m_temporaryDir.isValid());
    prefix = QFINDTESTDATA("images/");
    if (prefix.isEmpty())
        QFAIL("Can't find images directory!");
    writePrefix = m_temporaryDir.path() + QLatin1Char('/');
}

// Testing get/set functions
void tst_QImageWriter::getSetCheck()
{
    QImageWriter obj1;
    // QIODevice * QImageWriter::device()
    // void QImageWriter::setDevice(QIODevice *)
    QFile *var1 = new QFile;
    obj1.setDevice(var1);

    QCOMPARE((QIODevice *) var1, obj1.device());
    // The class should possibly handle a 0-pointer as a device, since
    // there is a default contructor, so it's "handling" a 0 device by default.
    // For example: QMovie::setDevice(0) works just fine
    obj1.setDevice((QIODevice *)0);
    QCOMPARE((QIODevice *) 0, obj1.device());
    delete var1;

    // int QImageWriter::quality()
    // void QImageWriter::setQuality(int)
    obj1.setQuality(0);
    QCOMPARE(0, obj1.quality());
    obj1.setQuality(INT_MIN);
    QCOMPARE(INT_MIN, obj1.quality());
    obj1.setQuality(INT_MAX);
    QCOMPARE(INT_MAX, obj1.quality());

    // int QImageWriter::compression()
    // void QImageWriter::setCompression(int)
    obj1.setCompression(0);
    QCOMPARE(0, obj1.compression());
    obj1.setCompression(INT_MIN);
    QCOMPARE(INT_MIN, obj1.compression());
    obj1.setCompression(INT_MAX);
    QCOMPARE(INT_MAX, obj1.compression());

    // float QImageWriter::gamma()
    // void QImageWriter::setGamma(float)
    obj1.setGamma(0.0f);
    QCOMPARE(0.0f, obj1.gamma());
    obj1.setGamma(1.1f);
    QCOMPARE(1.1f, obj1.gamma());
}

void tst_QImageWriter::writeImage_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<bool>("lossy");
    QTest::addColumn<QByteArray>("format");

    QTest::newRow("BMP: colorful") << QString("colorful.bmp") << false << QByteArray("bmp");
    QTest::newRow("BMP: font") << QString("font.bmp") << false << QByteArray("bmp");
    QTest::newRow("XPM: marble") << QString("marble.xpm") << false << QByteArray("xpm");
    QTest::newRow("PNG: kollada") << QString("kollada.png") << false << QByteArray("png");
    QTest::newRow("PPM: teapot") << QString("teapot.ppm") << false << QByteArray("ppm");
    QTest::newRow("PBM: ship63") << QString("ship63.pbm") << true << QByteArray("pbm");
    QTest::newRow("XBM: gnus") << QString("gnus.xbm") << false << QByteArray("xbm");
    QTest::newRow("JPEG: beavis") << QString("beavis.jpg") << true << QByteArray("jpeg");
    QTest::newRow("ICO: App") << QString("App.ico") << true << QByteArray("ico");
}

void tst_QImageWriter::writeImage()
{
    QFETCH(QString, fileName);
    QFETCH(bool, lossy);
    QFETCH(QByteArray, format);

    SKIP_IF_UNSUPPORTED(format);

    QImage image;
    {
        QImageReader reader(prefix + fileName);
        image = reader.read();
        QVERIFY2(!image.isNull(), qPrintable(reader.errorString()));
    }
    {
        QImageWriter writer(writePrefix + "gen-" + fileName, format);
        QVERIFY(writer.write(image));
    }

    {
        bool skip = false;
#if defined(Q_OS_UNIX)
        if (::geteuid() == 0)
            skip = true;
#endif
        if (!skip) {
            // Shouldn't be able to write to read-only file
            QFile sourceFile(writePrefix + "gen-" + fileName);
            QFile::Permissions permissions = sourceFile.permissions();
            QVERIFY(sourceFile.setPermissions(QFile::ReadOwner | QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther));

            QImageWriter writer(writePrefix + "gen-" + fileName, format);
            QVERIFY(!writer.write(image));

            QVERIFY(sourceFile.setPermissions(permissions));
        }
    }

    QImage image2;
    {
        QImageReader reader(writePrefix + "gen-" + fileName);
        image2 = reader.read();
        QVERIFY(!image2.isNull());
    }
    if (!lossy) {
        QCOMPARE(image, image2);
    } else {
        QCOMPARE(image.format(), image2.format());
        QCOMPARE(image.depth(), image2.depth());
    }
}

void tst_QImageWriter::writeImage2_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QByteArray>("format");
    QTest::addColumn<QImage>("image");

    static const QLatin1String formats[] = {
        QLatin1String("bmp"),
        QLatin1String("xpm"),
        QLatin1String("png"),
        QLatin1String("ppm"),
        QLatin1String("ico"),
        // QLatin1String("jpeg"),
    };

    QImage image0(70, 70, QImage::Format_RGB32);
    image0.fill(QColor(Qt::red).rgb());

    QImage::Format imgFormat = QImage::Format_Mono;
    while (imgFormat != QImage::NImageFormats) {
        QImage image = image0.convertToFormat(imgFormat);
        initializePadding(&image);
        for (QLatin1String format : formats) {
            const QString fileName = QLatin1String("solidcolor_")
                + QString::number(imgFormat) + QLatin1Char('.') + format;
            QTest::newRow(fileName.toLatin1()) << writePrefix + fileName
                                               << QByteArray(format.data(), format.size())
                                               << image;
        }
        imgFormat = QImage::Format(int(imgFormat) + 1);
    }
}

/*
    Workaround for the equality operator for indexed formats
    (which fails if the colortables are different).

    Images must have the same format and size.
*/
static bool equalImageContents(const QImage &image1, const QImage &image2)
{
    switch (image1.format()) {
    case QImage::Format_Mono:
    case QImage::Format_Indexed8:
        for (int y = 0; y < image1.height(); ++y)
            for (int x = 0; x < image1.width(); ++x)
                if (image1.pixel(x, y) != image2.pixel(x, y))
                    return false;
        return true;
    default:
        return (image1 == image2);
    }
}

void tst_QImageWriter::writeImage2()
{
    QFETCH(QString, fileName);
    QFETCH(QByteArray, format);
    QFETCH(QImage, image);

    //we reduce the scope of writer so that it closes the associated file
    // and QFile::remove can actually work
    {
        QImageWriter writer(fileName, format);
        QVERIFY(writer.write(image));
    }

    QImage written;

    //we reduce the scope of reader so that it closes the associated file
    // and QFile::remove can actually work
    {
        QImageReader reader(fileName, format);
        QVERIFY(reader.read(&written));
    }

    written = written.convertToFormat(image.format());
    if (!equalImageContents(written, image)) {
        qDebug() << "image" << image.format() << image.width()
                 << image.height() << image.depth()
                 << image.pixelColor(0, 0);
        qDebug() << "written" << written.format() << written.width()
                 << written.height() << written.depth()
                 << written.pixelColor(0, 0);
    }
    QVERIFY(equalImageContents(written, image));

    QVERIFY(QFile::remove(fileName));
}

namespace {
// C++98-library version of C++11 std::is_sorted
template <typename C>
bool is_sorted(const C &c)
{
    return std::adjacent_find(c.begin(), c.end(), std::greater_equal<typename C::value_type>()) == c.end();
}
}

void tst_QImageWriter::supportedFormats()
{
    QList<QByteArray> formats = QImageWriter::supportedImageFormats();

    // check that the list is sorted
    QVERIFY(is_sorted(formats));

    // check that the list does not contain duplicates
    QVERIFY(std::unique(formats.begin(), formats.end()) == formats.end());
}

void tst_QImageWriter::supportedMimeTypes()
{
    QList<QByteArray> mimeTypes = QImageWriter::supportedMimeTypes();

    // check that the list is sorted
    QVERIFY(is_sorted(mimeTypes));

    // check the list as a minimum contains image/bmp
    QVERIFY(mimeTypes.contains("image/bmp"));

    // check that the list does not contain duplicates
    QVERIFY(std::unique(mimeTypes.begin(), mimeTypes.end()) == mimeTypes.end());
}

void tst_QImageWriter::writeToInvalidDevice()
{
    QLatin1String fileName("/these/directories/do/not/exist/001.png");
    {
        QImageWriter writer(fileName);
        QVERIFY(!writer.canWrite());
        QCOMPARE(writer.error(), QImageWriter::DeviceError);
    }
    {
        QImageWriter writer(fileName);
        writer.setFormat("png");
        QVERIFY(!writer.canWrite());
        QCOMPARE(writer.error(), QImageWriter::DeviceError);
    }
    {
        QImageWriter writer(fileName);
        QImage im(10, 10, QImage::Format_ARGB32);
        QVERIFY(!writer.write(im));
        QCOMPARE(writer.error(), QImageWriter::DeviceError);
    }
    {
        QImageWriter writer(fileName);
        writer.setFormat("png");
        QImage im(10, 10, QImage::Format_ARGB32);
        QVERIFY(!writer.write(im));
        QCOMPARE(writer.error(), QImageWriter::DeviceError);
    }
}

void tst_QImageWriter::testCanWrite()
{
    {
        // device is not set
        QImageWriter writer;
        QVERIFY(!writer.canWrite());
        QCOMPARE(writer.error(), QImageWriter::DeviceError);
    }

    {
        // check if canWrite won't leave an empty file
        QTemporaryDir dir;
        QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));
        QString fileName(dir.path() + QLatin1String("/001.garble"));
        QVERIFY(!QFileInfo(fileName).exists());
        QImageWriter writer(fileName);
        QVERIFY(!writer.canWrite());
        QCOMPARE(writer.error(), QImageWriter::UnsupportedFormatError);
        QVERIFY(!QFileInfo(fileName).exists());
    }
}

void tst_QImageWriter::supportsOption_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QIntList>("options");

    QTest::newRow("png") << QString("gen-black.png")
                         << (QIntList() << QImageIOHandler::Gamma
                              << QImageIOHandler::Description
                              << QImageIOHandler::Quality
                              << QImageIOHandler::CompressionRatio
                              << QImageIOHandler::Size
                              << QImageIOHandler::ScaledSize);
}

void tst_QImageWriter::supportsOption()
{
    SKIP_IF_UNSUPPORTED(QTest::currentDataTag());

    QFETCH(QString, fileName);
    QFETCH(QIntList, options);

    static Q_CONSTEXPR QImageIOHandler::ImageOption allOptions[] = {
        QImageIOHandler::Size,
        QImageIOHandler::ClipRect,
        QImageIOHandler::Description,
        QImageIOHandler::ScaledClipRect,
        QImageIOHandler::ScaledSize,
        QImageIOHandler::CompressionRatio,
        QImageIOHandler::Gamma,
        QImageIOHandler::Quality,
        QImageIOHandler::Name,
        QImageIOHandler::SubType,
        QImageIOHandler::IncrementalReading,
        QImageIOHandler::Endianness,
        QImageIOHandler::Animation,
        QImageIOHandler::BackgroundColor,
    };

    QImageWriter writer(writePrefix + fileName);
    for (auto option : allOptions) {
        QCOMPARE(writer.supportsOption(option), options.contains(option));
    }
}

void tst_QImageWriter::saveWithNoFormat_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QByteArray>("format");
    QTest::addColumn<QImageWriter::ImageWriterError>("error");

    QTest::newRow("garble") << writePrefix + QString("gen-out.garble") << QByteArray("jpeg") << QImageWriter::UnsupportedFormatError;
    QTest::newRow("bmp") << writePrefix + QString("gen-out.bmp") << QByteArray("bmp") << QImageWriter::ImageWriterError(0);
    QTest::newRow("xbm") << writePrefix + QString("gen-out.xbm") << QByteArray("xbm") << QImageWriter::ImageWriterError(0);
    QTest::newRow("xpm") << writePrefix + QString("gen-out.xpm") << QByteArray("xpm") << QImageWriter::ImageWriterError(0);
    QTest::newRow("png") << writePrefix + QString("gen-out.png") << QByteArray("png") << QImageWriter::ImageWriterError(0);
    QTest::newRow("ppm") << writePrefix + QString("gen-out.ppm") << QByteArray("ppm") << QImageWriter::ImageWriterError(0);
    QTest::newRow("pbm") << writePrefix + QString("gen-out.pbm") << QByteArray("pbm") << QImageWriter::ImageWriterError(0);
}

void tst_QImageWriter::saveWithNoFormat()
{
    QFETCH(QString, fileName);
    QFETCH(QByteArray, format);
    QFETCH(QImageWriter::ImageWriterError, error);

    SKIP_IF_UNSUPPORTED(format);

    QImage niceImage(64, 64, QImage::Format_ARGB32);
    memset(niceImage.bits(), 0, niceImage.sizeInBytes());

    QImageWriter writer(fileName /* , 0 - no format! */);
    if (error != 0) {
        QVERIFY(!writer.write(niceImage));
        QCOMPARE(writer.error(), error);
        return;
    }

    QVERIFY2(writer.write(niceImage), qPrintable(writer.errorString()));

    QImageReader reader(fileName);
    QCOMPARE(reader.format(), format);

    QVERIFY(reader.canRead());

    QImage outImage = reader.read();
    QVERIFY2(!outImage.isNull(), qPrintable(reader.errorString()));
}

void tst_QImageWriter::saveToTemporaryFile()
{
    QImage image(prefix + "kollada.png");
    QVERIFY(!image.isNull());

    {
        // 1) Via QImageWriter's API, with a standard temp file name
        QTemporaryFile file;
        QVERIFY2(file.open(), qPrintable(file.errorString()));
        QImageWriter writer(&file, "PNG");
        if (writer.canWrite())
            QVERIFY(writer.write(image));
        else
            qWarning() << file.errorString();
        QCOMPARE(QImage(writer.fileName()), image);
    }
    {
        // 2) Via QImage's API, with a standard temp file name
        QTemporaryFile file;
        QVERIFY2(file.open(), qPrintable(file.errorString()));
        QVERIFY(image.save(&file, "PNG"));
        file.reset();
        QImage tmp;
        QVERIFY(tmp.load(&file, "PNG"));
        QCOMPARE(tmp, image);
    }
    {
        // 3) Via QImageWriter's API, with a named temp file
        QTemporaryFile file(writePrefix + QLatin1String("tempXXXXXX"));
        QVERIFY2(file.open(), qPrintable(file.errorString()));
        QImageWriter writer(&file, "PNG");
        QVERIFY(writer.write(image));
        QCOMPARE(QImage(writer.fileName()), image);
    }
    {
        // 4) Via QImage's API, with a named temp file
        QTemporaryFile file(writePrefix + QLatin1String("tempXXXXXX"));
        QVERIFY2(file.open(), qPrintable(file.errorString()));
        QVERIFY(image.save(&file, "PNG"));
        file.reset();
        QImage tmp;
        QVERIFY(tmp.load(&file, "PNG"));
        QCOMPARE(tmp, image);
    }
}

void tst_QImageWriter::writeEmpty()
{
    // check writing a null QImage errors gracefully
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));
    QString fileName(dir.path() + QLatin1String("/testimage.bmp"));
    QVERIFY(!QFileInfo(fileName).exists());
    QImageWriter writer(fileName);
    QVERIFY(!writer.write(QImage()));
    QCOMPARE(writer.error(), QImageWriter::InvalidImageError);
    QVERIFY(!QFileInfo(fileName).exists());
}

QTEST_MAIN(tst_QImageWriter)
#include "tst_qimagewriter.moc"
