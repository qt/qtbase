// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QImageReader>
#include <QBuffer>
#include <QStandardPaths>
#include <QPainter>
#if QT_CONFIG(process)
#include <QProcess>
#endif
#include <qicon.h>
#include <private/qabstractfileiconengine_p.h>

#include <algorithm>

class tst_QIcon : public QObject
{
    Q_OBJECT
public:
    tst_QIcon();

private slots:
    void initTestCase();
    void actualSize_data(); // test with 1 pixmap
    void actualSize();
    void actualSize2_data(); // test with 2 pixmaps with different aspect ratio
    void actualSize2();
    void isNull();
    void isMask();
    void swap();
    void bestMatch();
    void cacheKey();
    void detach();
    void addFile();
    void pixmap();
    void pixmapByDprFromEngine_data();
    void pixmapByDprFromEngine();
    void paint();
    void availableSizes();
    void name();
    void streamAvailableSizes_data();
    void streamAvailableSizes();
    void fromTheme();
    void fromThemeCache();
    void fromThemeConstant();

#ifndef QT_NO_WIDGETS
    void task184901_badCache();
#endif
    void task223279_inconsistentAddFile();

    void themeFromPlugin_data();
    void themeFromPlugin();

private:
    bool haveImageFormat(QByteArray const&);

    const QString m_pngImageFileName;
    const QString m_pngRectFileName;
    const QString m_sourceFileName;
};

bool tst_QIcon::haveImageFormat(QByteArray const& desiredFormat)
{
    return QImageReader::supportedImageFormats().contains(desiredFormat);
}

tst_QIcon::tst_QIcon()
    : m_pngImageFileName(QFINDTESTDATA("image.png"))
    , m_pngRectFileName(QFINDTESTDATA("rect.png"))
    , m_sourceFileName(":/tst_qicon.cpp")
{
}

void tst_QIcon::initTestCase()
{
    QVERIFY(!m_pngImageFileName.isEmpty());
    QVERIFY(!m_pngRectFileName.isEmpty());
    QVERIFY(!m_sourceFileName.isEmpty());
}

void tst_QIcon::actualSize_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QSize>("argument");
    QTest::addColumn<QSize>("result");

    // square image
    QTest::newRow("resource0") << ":/image.png" << QSize(128, 128) << QSize(128, 128);
    QTest::newRow("resource1") << ":/image.png" << QSize( 64,  64) << QSize( 64,  64);
    QTest::newRow("resource2") << ":/image.png" << QSize( 32,  64) << QSize( 32,  32);
    QTest::newRow("resource3") << ":/image.png" << QSize( 16,  64) << QSize( 16,  16);
    QTest::newRow("resource4") << ":/image.png" << QSize( 16,  128) << QSize( 16,  16);
    QTest::newRow("resource5") << ":/image.png" << QSize( 128,  16) << QSize( 16,  16);
    QTest::newRow("resource6") << ":/image.png" << QSize( 150,  150) << QSize( 128,  128);
    // rect image
    QTest::newRow("resource7") << ":/rect.png" << QSize( 20,  40) << QSize( 20,  40);
    QTest::newRow("resource8") << ":/rect.png" << QSize( 10,  20) << QSize( 10,  20);
    QTest::newRow("resource9") << ":/rect.png" << QSize( 15,  50) << QSize( 15,  30);
    QTest::newRow("resource10") << ":/rect.png" << QSize( 25,  50) << QSize( 20,  40);

    QTest::newRow("external0") << m_pngImageFileName << QSize(128, 128) << QSize(128, 128);
    QTest::newRow("external1") << m_pngImageFileName << QSize( 64,  64) << QSize( 64,  64);
    QTest::newRow("external2") << m_pngImageFileName << QSize( 32,  64) << QSize( 32,  32);
    QTest::newRow("external3") << m_pngImageFileName << QSize( 16,  64) << QSize( 16,  16);
    QTest::newRow("external4") << m_pngImageFileName << QSize( 16, 128) << QSize( 16,  16);
    QTest::newRow("external5") << m_pngImageFileName << QSize(128,  16) << QSize( 16,  16);
    QTest::newRow("external6") << m_pngImageFileName << QSize(150, 150) << QSize(128,  128);
    // rect image
    QTest::newRow("external7") << ":/rect.png" << QSize( 20,  40) << QSize( 20,  40);
    QTest::newRow("external8") << ":/rect.png" << QSize( 10,  20) << QSize( 10,  20);
    QTest::newRow("external9") << ":/rect.png" << QSize( 15,  50) << QSize( 15,  30);
    QTest::newRow("external10") << ":/rect.png" << QSize( 25,  50) << QSize( 20,  40);
}

void tst_QIcon::actualSize()
{
    QFETCH(QString, source);
    QFETCH(QSize, argument);
    QFETCH(QSize, result);

    // Skip two corner cases
    if (qApp->devicePixelRatio() > 1 && (qstrcmp(QTest::currentDataTag(), "resource9") == 0
                                      || qstrcmp(QTest::currentDataTag(), "external9") == 0))
        QSKIP("Behavior is unspecified for devicePixelRatio > 1");

    auto expectedDeviceSize = [](QSize deviceIndependentExpectedSize, QSize maxSourceImageSize) -> QSize {
        qreal dpr = qApp->devicePixelRatio();
        return QSize(qMin(qRound(deviceIndependentExpectedSize.width() * dpr), maxSourceImageSize.width()),
                     qMin(qRound(deviceIndependentExpectedSize.height() * dpr), maxSourceImageSize.height()));
    };

    QSize sourceSize = QImage(source).size();
    QSize deviceIndependentSize = result;
    QSize deviceSize = expectedDeviceSize(result, sourceSize);

    {
        QPixmap pixmap(source);
        QIcon icon(pixmap);
        QCOMPARE(icon.actualSize(argument), deviceIndependentSize);
        QCOMPARE(icon.pixmap(argument).size(), deviceSize);
    }

    {
        QIcon icon(source);
        QCOMPARE(icon.actualSize(argument), deviceIndependentSize);
        QCOMPARE(icon.pixmap(argument).size(), deviceSize);
    }
}

void tst_QIcon::actualSize2_data()
{
    QTest::addColumn<QSize>("argument");
    QTest::addColumn<QSize>("result");

    // two images - 128x128 and 20x40. Let the games begin
    QTest::newRow("trivial1") << QSize( 128,  128) << QSize( 128,  128);
    QTest::newRow("trivial2") << QSize( 20,  40) << QSize( 20,  40);

    // QIcon chooses the one with the smallest area to choose the pixmap
    QTest::newRow("best1") << QSize( 100,  100) << QSize( 100,  100);
    QTest::newRow("best2") << QSize( 20,  20) << QSize( 10,  20);
    QTest::newRow("best3") << QSize( 15,  30) << QSize( 15,  30);
    QTest::newRow("best4") << QSize( 5,  5) << QSize( 2,  5);
    QTest::newRow("best5") << QSize( 10,  15) << QSize( 7,  15);
}

void tst_QIcon::actualSize2()
{
    if (qApp->devicePixelRatio() > 1)
        QSKIP("Behavior is unspecified for devicePixelRatio > 1");

    QIcon icon;
    icon.addPixmap(m_pngImageFileName);
    icon.addPixmap(m_pngRectFileName);

    QFETCH(QSize, argument);
    QFETCH(QSize, result);

    QCOMPARE(icon.actualSize(argument), result);
    QCOMPARE(icon.pixmap(argument).size(), result);
}

void tst_QIcon::isNull() {
    // test default constructor
    QIcon defaultConstructor;
    QVERIFY(defaultConstructor.isNull());

    // test copy constructor
    QVERIFY(QIcon(defaultConstructor).isNull());

    // test pixmap constructor
    QPixmap nullPixmap;
    QVERIFY(QIcon(nullPixmap).isNull());

    // test string constructor with empty string
    QIcon iconEmptyString = QIcon(QString());
    QVERIFY(iconEmptyString.isNull());
    QVERIFY(!iconEmptyString.actualSize(QSize(32, 32)).isValid());

    // test string constructor with non-existing file
    QIcon iconNoFile = QIcon("imagedoesnotexist");
    QVERIFY(iconNoFile.isNull());
    QVERIFY(!iconNoFile.actualSize(QSize(32, 32)).isValid());

    // test string constructor with non-existing file with suffix
    QIcon iconNoFileSuffix = QIcon("imagedoesnotexist.png");
    QVERIFY(iconNoFileSuffix.isNull());
    QVERIFY(!iconNoFileSuffix.actualSize(QSize(32, 32)).isValid());

    // test string constructor with existing file but unsupported format
    QIcon iconUnsupportedFormat = QIcon(m_sourceFileName);
    QVERIFY(iconUnsupportedFormat.isNull());
    QVERIFY(!iconUnsupportedFormat.actualSize(QSize(32, 32)).isValid());

    // test string constructor with existing file and supported format
    QIcon iconSupportedFormat = QIcon(m_pngImageFileName);
    QVERIFY(!iconSupportedFormat.isNull());
    QVERIFY(iconSupportedFormat.actualSize(QSize(32, 32)).isValid());
}

void tst_QIcon::isMask()
{
    QIcon icon;
    icon.setIsMask(true);
    icon.addPixmap(QPixmap());
    QVERIFY(icon.isMask());

    QIcon icon2;
    icon2.setIsMask(true);
    QVERIFY(icon2.isMask());
    icon2.setIsMask(false);
    QVERIFY(!icon2.isMask());
}

void tst_QIcon::swap()
{
    QPixmap p1(1, 1), p2(2, 2);
    p1.fill(Qt::black);
    p2.fill(Qt::black);

    QIcon i1(p1), i2(p2);
    const qint64 i1k = i1.cacheKey();
    const qint64 i2k = i2.cacheKey();
    QVERIFY(i1k != i2k);
    i1.swap(i2);
    QCOMPARE(i1.cacheKey(), i2k);
    QCOMPARE(i2.cacheKey(), i1k);
}

void tst_QIcon::bestMatch()
{
    QPixmap p1(1, 1);
    QPixmap p2(2, 2);
    QPixmap p3(3, 3);
    QPixmap p4(4, 4);
    QPixmap p5(5, 5);
    QPixmap p6(6, 6);
    QPixmap p7(7, 7);
    QPixmap p8(8, 8);

    p1.fill(Qt::black);
    p2.fill(Qt::black);
    p3.fill(Qt::black);
    p4.fill(Qt::black);
    p5.fill(Qt::black);
    p6.fill(Qt::black);
    p7.fill(Qt::black);
    p8.fill(Qt::black);

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 2; ++j) {
            QIcon::State state = (j == 0) ? QIcon::On : QIcon::Off;
            QIcon::State oppositeState = (state == QIcon::On) ? QIcon::Off
                                                              : QIcon::On;
            QIcon::Mode mode;
            QIcon::Mode oppositeMode;

            QIcon icon;

            switch (i) {
            case 0:
            default:
                mode = QIcon::Normal;
                oppositeMode = QIcon::Active;
                break;
            case 1:
                mode = QIcon::Active;
                oppositeMode = QIcon::Normal;
                break;
            case 2:
                mode = QIcon::Disabled;
                oppositeMode = QIcon::Selected;
                break;
            case 3:
                mode = QIcon::Selected;
                oppositeMode = QIcon::Disabled;
            }

            /*
                The test mirrors the code in
                QPixmapIconEngine::bestMatch(), to make sure that
                nobody breaks QPixmapIconEngine by mistake. Before
                you change this test or the code that it tests,
                please talk to the maintainer if possible.
            */
            if (mode == QIcon::Disabled || mode == QIcon::Selected) {
                icon.addPixmap(p1, oppositeMode, oppositeState);
                QVERIFY(icon.pixmap(100, mode, state).size() == p1.size());

                icon.addPixmap(p2, oppositeMode, state);
                QVERIFY(icon.pixmap(100, mode, state).size() == p2.size());

                icon.addPixmap(p3, QIcon::Active, oppositeState);
                QVERIFY(icon.pixmap(100, mode, state).size() == p3.size());

                icon.addPixmap(p4, QIcon::Normal, oppositeState);
                QVERIFY(icon.pixmap(100, mode, state).size() == p4.size());

                icon.addPixmap(p5, mode, oppositeState);
                QVERIFY(icon.pixmap(100, mode, state).size() == p5.size());

                icon.addPixmap(p6, QIcon::Active, state);
                QVERIFY(icon.pixmap(100, mode, state).size() == p6.size());

                icon.addPixmap(p7, QIcon::Normal, state);
                QVERIFY(icon.pixmap(100, mode, state).size() == p7.size());

                icon.addPixmap(p8, mode, state);
                QVERIFY(icon.pixmap(100, mode, state).size() == p8.size());
            } else {
                icon.addPixmap(p1, QIcon::Selected, oppositeState);
                QVERIFY(icon.pixmap(100, mode, state).size() == p1.size());

                icon.addPixmap(p2, QIcon::Disabled, oppositeState);
                QVERIFY(icon.pixmap(100, mode, state).size() == p2.size());

                icon.addPixmap(p3, QIcon::Selected, state);
                QVERIFY(icon.pixmap(100, mode, state).size() == p3.size());

                icon.addPixmap(p4, QIcon::Disabled, state);
                QVERIFY(icon.pixmap(100, mode, state).size() == p4.size());

                icon.addPixmap(p5, oppositeMode, oppositeState);
                QVERIFY(icon.pixmap(100, mode, state).size() == p5.size());

                icon.addPixmap(p6, mode, oppositeState);
                QVERIFY(icon.pixmap(100, mode, state).size() == p6.size());

                icon.addPixmap(p7, oppositeMode, state);
                QVERIFY(icon.pixmap(100, mode, state).size() == p7.size());

                icon.addPixmap(p8, mode, state);
                QVERIFY(icon.pixmap(100, mode, state).size() == p8.size());
            }
        }
    }
}

void tst_QIcon::cacheKey()
{
    QIcon icon1(m_pngImageFileName);
    qint64 icon1_key = icon1.cacheKey();
    QIcon icon2 = icon1;

    QCOMPARE(icon2.cacheKey(), icon1.cacheKey());
    icon2.detach();
    QVERIFY(icon2.cacheKey() != icon1.cacheKey());
    QCOMPARE(icon1.cacheKey(), icon1_key);
}

void tst_QIcon::detach()
{
    QImage img(32, 32, QImage::Format_ARGB32_Premultiplied);
    img.fill(0xffff0000);
    QIcon icon1(QPixmap::fromImage(img));
    QIcon icon2 = icon1;
    icon2.addFile(m_pngImageFileName, QSize(64, 64));

    QImage img1 = icon1.pixmap(64, 64).toImage();
    QImage img2 = icon2.pixmap(64, 64).toImage();
    QVERIFY(img1 != img2);

    img1 = icon1.pixmap(32, 32).toImage();
    img2 = icon2.pixmap(32, 32).toImage();

    if (qApp->devicePixelRatio() > 1)
        QVERIFY(img1 != img2); // we get an e.g. 64x64 image in dpr=2 displays
    else
        QCOMPARE(img1, img2);
}

void tst_QIcon::addFile()
{
    if (qApp->devicePixelRatio() != int(qApp->devicePixelRatio()))
        QSKIP("Test is not ready for non integer devicePixelRatio");

    QIcon icon;
    icon.addFile(QLatin1String(":/styles/commonstyle/images/standardbutton-open-16.png"));
    icon.addFile(QLatin1String(":/styles/commonstyle/images/standardbutton-open-32.png"));
    icon.addFile(QLatin1String(":/styles/commonstyle/images/standardbutton-open-64.png"));
    icon.addFile(QLatin1String(":/styles/commonstyle/images/standardbutton-open-128.png"));
    icon.addFile(QLatin1String(":/styles/commonstyle/images/standardbutton-save-16.png"), QSize(), QIcon::Selected);
    icon.addFile(QLatin1String(":/styles/commonstyle/images/standardbutton-save-32.png"), QSize(), QIcon::Selected);
    icon.addFile(QLatin1String(":/styles/commonstyle/images/standardbutton-save-64.png"), QSize(), QIcon::Selected);
    icon.addFile(QLatin1String(":/styles/commonstyle/images/standardbutton-save-128.png"), QSize(), QIcon::Selected);

    const int maxImageSize = 128;

    auto expectedHighDpiImage = [=](int deviceIndependentSize, const QString &imagePathTemplate) -> QImage {
        const int expectedImageSize = qMin(maxImageSize, deviceIndependentSize * qCeil(qApp->devicePixelRatio()));
        const int expectedImageDpr = expectedImageSize / deviceIndependentSize;
        const QString path = imagePathTemplate.arg(expectedImageSize);
        QPixmap image(path);
        image.setDevicePixelRatio(expectedImageDpr);
        return image.toImage();
    };

    QCOMPARE(icon.pixmap(16, QIcon::Normal).toImage(),
        expectedHighDpiImage(16, ":/styles/commonstyle/images/standardbutton-open-%1.png"));
    QCOMPARE(icon.pixmap(32, QIcon::Normal).toImage(),
        expectedHighDpiImage(32, ":/styles/commonstyle/images/standardbutton-open-%1.png"));
    QCOMPARE(icon.pixmap(64, QIcon::Normal).toImage(),
        expectedHighDpiImage(64, ":/styles/commonstyle/images/standardbutton-open-%1.png"));
    QCOMPARE(icon.pixmap(128, QIcon::Normal).toImage(),
        expectedHighDpiImage(128, ":/styles/commonstyle/images/standardbutton-open-%1.png"));

    QCOMPARE(icon.pixmap(16, QIcon::Selected).toImage(),
        expectedHighDpiImage(16, ":/styles/commonstyle/images/standardbutton-save-%1.png"));
    QCOMPARE(icon.pixmap(32, QIcon::Selected).toImage(),
        expectedHighDpiImage(32, ":/styles/commonstyle/images/standardbutton-save-%1.png"));
    QCOMPARE(icon.pixmap(64, QIcon::Selected).toImage(),
        expectedHighDpiImage(64, ":/styles/commonstyle/images/standardbutton-save-%1.png"));
    QCOMPARE(icon.pixmap(128, QIcon::Selected).toImage(),
        expectedHighDpiImage(128, ":/styles/commonstyle/images/standardbutton-save-%1.png"));
}

void tst_QIcon::pixmap()
{
    QIcon icon;
    icon.addFile(m_pngImageFileName, QSize(64, 64));

    // Exercise all pixmap() API overloads
    QVERIFY(icon.pixmap(16).size().width() >= 16);
    QVERIFY(icon.pixmap(16, 16).size().width() >= 16);
    QVERIFY(icon.pixmap(QSize(16, 16)).size().width() >= 16);
    QVERIFY(icon.pixmap(QSize(16, 16), 1).size().width() == 16);
    QVERIFY(icon.pixmap(QSize(16, 16), -1).size().width() >= 16);
}

void tst_QIcon::pixmapByDprFromEngine_data()
{
    QTest::addColumn<int>("engineSize");
    QTest::addColumn<int>("requestedSize");
    QTest::addColumn<qreal>("requestedDpr");
    QTest::addColumn<int>("expectedSize");
    QTest::addColumn<qreal>("expectedDpr");

    QTest::newRow("engine 16x16, request 32x32, dpr = 1")
        << 16 << 32 << 1.0 << 16 << 1.0;    // no upscaling is done
    QTest::newRow("engine 16x16, request 32x32, dpr = 2")
        << 16 << 32 << 2.0 << 16 << 1.0;    // no upscaling is done
    QTest::newRow("engine 32x32, request 32x32, dpr = 1")
        << 32 << 32 << 1.0 << 32 << 1.0;
    QTest::newRow("engine 32x32, request 32x32, dpr = 2")
        << 32 << 32 << 2.0 << 32 << 1.0;    // no upscaling is done
    QTest::newRow("engine 32x32, request 16x16, dpr = 1")
        << 32 << 16 << 1.0 << 32 << 2.0;    // downscaling done by increasing dpr
    QTest::newRow("engine 32x32, request 16x16, dpr = 2")
        << 32 << 16 << 2.0 << 32 << 2.0;
    QTest::newRow("engine 32x32, request 8x8, dpr = 1")
        << 32 << 8 << 1.0 << 32 << 4.0;     // downscaling done by increasing dpr
    QTest::newRow("engine 32x32, request 8x8, dpr = 2")
        << 32 << 8 << 2.0 << 32 << 4.0;     // downscaling done by increasing dpr
}

void tst_QIcon::pixmapByDprFromEngine()
{
    QFETCH(int, engineSize);
    QFETCH(int, requestedSize);
    QFETCH(qreal, requestedDpr);
    QFETCH(int, expectedSize);
    QFETCH(qreal, expectedDpr);

    class TestEngine : public QPixmapIconEngine
    {
    public:
        using QPixmapIconEngine::QPixmapIconEngine;
        QSize size;

        QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override
        {
            return scaledPixmap(size, mode, state, 1.0f);
        }
        QPixmap scaledPixmap(const QSize &, QIcon::Mode, QIcon::State, qreal) override
        {
            // simulate an icon engine which does no scaling (= only has fixed size icons)
            QPixmap pm(size);
            pm.fill(Qt::red);
            return pm;
        }
    };

    auto testEngine = new TestEngine;
    QIcon ico(testEngine);
    testEngine->size = QSize(engineSize, engineSize);
    auto pm = ico.pixmap(QSize(requestedSize, requestedSize), requestedDpr);
    QCOMPARE(pm.size(), QSize(expectedSize, expectedSize));
    QCOMPARE(pm.devicePixelRatio(), expectedDpr);
}

void tst_QIcon::paint()
{
    QImage img16_1x(16, 16, QImage::Format_ARGB32);
    img16_1x.fill(qRgb(0, 0, 0xff));
    img16_1x.setDevicePixelRatio(1.);

    QImage img16_2x(32, 32, QImage::Format_ARGB32);
    img16_2x.fill(qRgb(0, 0xff, 0xff));
    img16_2x.setDevicePixelRatio(2.);

    QImage img32_1x(32, 32, QImage::Format_ARGB32);
    img32_1x.fill(qRgb(0xff, 0, 0));
    img32_1x.setDevicePixelRatio(1.);

    QImage img32_2x(64, 64, QImage::Format_ARGB32);
    img32_2x.fill(qRgb(0x0, 0xff, 0));
    img32_2x.setDevicePixelRatio(2.);

    QIcon icon;
    icon.addPixmap(QPixmap::fromImage(img16_1x));
    icon.addPixmap(QPixmap::fromImage(img16_2x));
    icon.addPixmap(QPixmap::fromImage(img32_1x));
    icon.addPixmap(QPixmap::fromImage(img32_2x));

    // Test painting the icon version with a device independent size of 32x32
    QRect iconRect(0, 0, 32, 32);

    auto imageWithPaintedIconAtDpr = [&](qreal dpr) {
        QImage paintDevice(64 * dpr, 64 * dpr, QImage::Format_ARGB32);
        paintDevice.setDevicePixelRatio(dpr);

        QPainter painter(&paintDevice);
        icon.paint(&painter, iconRect);
        return paintDevice;
    };

    QImage imageWithIcon1x = imageWithPaintedIconAtDpr(1.0);
    QCOMPARE(imageWithIcon1x.pixel(iconRect.center()), qRgb(0xff, 0, 0));

    QImage imageWithIcon2x = imageWithPaintedIconAtDpr(2.0);
    QCOMPARE(imageWithIcon2x.pixel(iconRect.center()), qRgb(0, 0xff, 0));

    QImage imageWithIcon3x = imageWithPaintedIconAtDpr(3.0);
    QCOMPARE(imageWithIcon3x.pixel(iconRect.center()), qRgb(0, 0xff, 0));
}

static bool sizeLess(const QSize &a, const QSize &b)
{
    return a.width() < b.width();
}

void tst_QIcon::availableSizes()
{
    {
        QIcon icon;
        icon.addFile(m_pngImageFileName, QSize(32,32));
        icon.addFile(m_pngImageFileName, QSize(64,64));
        icon.addFile(m_pngImageFileName, QSize(128,128));
        icon.addFile(m_pngImageFileName, QSize(256,256), QIcon::Disabled);
        icon.addFile(m_pngImageFileName, QSize(16,16), QIcon::Normal, QIcon::On);

        QList<QSize> availableSizes = icon.availableSizes();
        QCOMPARE(availableSizes.size(), 3);
        std::sort(availableSizes.begin(), availableSizes.end(), sizeLess);
        QCOMPARE(availableSizes.at(0), QSize(32,32));
        QCOMPARE(availableSizes.at(1), QSize(64,64));
        QCOMPARE(availableSizes.at(2), QSize(128,128));

        availableSizes = icon.availableSizes(QIcon::Disabled);
        QCOMPARE(availableSizes.size(), 1);
        QCOMPARE(availableSizes.at(0), QSize(256,256));

        availableSizes = icon.availableSizes(QIcon::Normal, QIcon::On);
        QCOMPARE(availableSizes.size(), 1);
        QCOMPARE(availableSizes.at(0), QSize(16,16));
    }

    {
        // we try to load an icon from resources
        QIcon icon(QLatin1String(":/styles/commonstyle/images/standardbutton-open-16.png"));
        QList<QSize> availableSizes = icon.availableSizes();
        QCOMPARE(availableSizes.size(), 1);
        QCOMPARE(availableSizes.at(0), QSize(16, 16));
    }

    {
        // load an icon from binary data.
        QPixmap pix;
        QFile file(QLatin1String(":/styles/commonstyle/images/standardbutton-open-16.png"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        uchar *data = file.map(0, file.size());
        QVERIFY(data != 0);
        pix.loadFromData(data, file.size());
        QIcon icon(pix);

        QList<QSize> availableSizes = icon.availableSizes();
        QCOMPARE(availableSizes.size(), 1);
        QCOMPARE(availableSizes.at(0), QSize(16,16));
    }

    {
        // there shouldn't be available sizes for invalid images!
        QVERIFY(QIcon(QLatin1String("")).availableSizes().isEmpty());
        QVERIFY(QIcon(QLatin1String("non-existing.png")).availableSizes().isEmpty());
    }
}

void tst_QIcon::name()
{
    const auto reset = qScopeGuard([]{
        QIcon::setThemeName({});
        QIcon::setThemeSearchPaths({});
    });
    {
        // No name if icon does not come from a theme
        QIcon icon(":/image.png");
        QString name = icon.name();
        QVERIFY(name.isEmpty());
    }

    {
        // Getting the name of an icon coming from a theme should work
        QString searchPath = QLatin1String(":/icons");
        QIcon::setThemeSearchPaths(QStringList() << searchPath);
        QString themeName("testtheme");
        QIcon::setThemeName(themeName);

        QIcon icon = QIcon::fromTheme("appointment-new");
        QString name = icon.name();
        QCOMPARE(name, QLatin1String("appointment-new"));
    }
}

void tst_QIcon::streamAvailableSizes_data()
{
    QTest::addColumn<QIcon>("icon");

    QIcon icon;
    icon.addFile(":/image.png", QSize(32,32));
    QTest::newRow( "32x32" ) << icon;
    icon.addFile(":/image.png", QSize(64,64));
    QTest::newRow( "64x64" ) << icon;
    icon.addFile(":/image.png", QSize(128,128));
    QTest::newRow( "128x128" ) << icon;
    icon.addFile(":/image.png", QSize(256,256));
    QTest::newRow( "256x256" ) << icon;
}

void tst_QIcon::streamAvailableSizes()
{
    QFETCH(QIcon, icon);

    QByteArray ba;
    // write to QByteArray
    {
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        QDataStream stream(&buffer);
        stream << icon;
    }

    // read from QByteArray
    {
        QBuffer buffer(&ba);
        buffer.open(QIODevice::ReadOnly);
        QDataStream stream(&buffer);
        QIcon i;
        stream >> i;
        QCOMPARE(i.isNull(), icon.isNull());
        QCOMPARE(i.availableSizes(), icon.availableSizes());
    }
}

#ifndef QT_NO_WIDGETS
void tst_QIcon::task184901_badCache()
{
    QPixmap pm(m_pngImageFileName);
    QIcon icon(pm);

    //the disabled icon must have an effect (grayed)
    QVERIFY(icon.pixmap(32, QIcon::Normal).toImage() != icon.pixmap(32, QIcon::Disabled).toImage());

    icon.addPixmap(pm, QIcon::Disabled);
    //the disabled icon must now be the same as the normal one.
    QVERIFY( icon.pixmap(32, QIcon::Normal).toImage() == icon.pixmap(32, QIcon::Disabled).toImage() );
}
#endif

void tst_QIcon::fromTheme()
{
    const bool abIconFromPlatform = !QIcon::fromTheme("address-book-new").isNull();
    QString firstSearchPath = QLatin1String(":/icons");
    QString secondSearchPath = QLatin1String(":/second_icons");
    QIcon::setThemeSearchPaths(QStringList() << firstSearchPath << secondSearchPath);
    QCOMPARE(QIcon::themeSearchPaths().size(), 2);
    QCOMPARE(firstSearchPath, QIcon::themeSearchPaths()[0]);
    QCOMPARE(secondSearchPath, QIcon::themeSearchPaths()[1]);

    QString fallbackSearchPath = QStringLiteral(":/fallback_icons");
    QIcon::setFallbackSearchPaths(QStringList() << fallbackSearchPath);
    QCOMPARE(QIcon::fallbackSearchPaths().size(), 1);
    QCOMPARE(fallbackSearchPath, QIcon::fallbackSearchPaths().at(0));

    QString themeName("testtheme");
    QIcon::setThemeName(themeName);
    QCOMPARE(QIcon::themeName(), themeName);

    // Test normal icon
    QIcon appointmentIcon = QIcon::fromTheme("appointment-new");
    QVERIFY(!appointmentIcon.isNull());
    QVERIFY(!appointmentIcon.availableSizes(QIcon::Normal, QIcon::Off).isEmpty());
    QVERIFY(appointmentIcon.availableSizes().contains(QSize(16, 16)));
    QVERIFY(appointmentIcon.availableSizes().contains(QSize(32, 32)));
    QVERIFY(appointmentIcon.availableSizes().contains(QSize(22, 22)));

    // Test fallback to less specific icon
    QIcon specificAppointmentIcon = QIcon::fromTheme("appointment-new-specific");
    QVERIFY(!QIcon::hasThemeIcon("appointment-new-specific"));
    QVERIFY(QIcon::hasThemeIcon("appointment-new"));
    QCOMPARE(specificAppointmentIcon.name(), QString::fromLatin1("appointment-new"));
    QCOMPARE(specificAppointmentIcon.availableSizes(), appointmentIcon.availableSizes());
    QCOMPARE(specificAppointmentIcon.pixmap(32).cacheKey(), appointmentIcon.pixmap(32).cacheKey());

    // Test icon from parent theme
    QIcon abIcon = QIcon::fromTheme("address-book-new");
    QVERIFY(!abIcon.isNull());
    QVERIFY(QIcon::hasThemeIcon("address-book-new"));
    QVERIFY(!abIcon.availableSizes().isEmpty());

    // Test icon from fallback path
    QIcon fallbackIcon = QIcon::fromTheme("red");
    QVERIFY(!fallbackIcon.isNull());
    QVERIFY(QIcon::hasThemeIcon("red"));
    QCOMPARE(fallbackIcon.availableSizes().size(), 1);

    // Test non existing icon
    QIcon noIcon = QIcon::fromTheme("broken-icon");
    QVERIFY(noIcon.isNull());
    QVERIFY(!QIcon::hasThemeIcon("broken-icon"));
    QCOMPARE(noIcon.actualSize(QSize(32, 32), QIcon::Normal, QIcon::On), QSize(0, 0));

    // Test non existing icon with fallback
    noIcon = QIcon::fromTheme("broken-icon", abIcon);
    QCOMPARE(noIcon.cacheKey(), abIcon.cacheKey());

    // Test svg-only icon
    noIcon = QIcon::fromTheme("svg-icon", abIcon);
    QVERIFY(!noIcon.availableSizes().isEmpty());

    // Pixmaps should be no larger than the requested size (for devicePixelRatio 1) (QTBUG-17953)
    if (qApp->devicePixelRatio() == 1) {
        QCOMPARE(appointmentIcon.pixmap(22).size(), QSize(22, 22)); // exact
        QCOMPARE(appointmentIcon.pixmap(32).size(), QSize(32, 32)); // exact
        QCOMPARE(appointmentIcon.pixmap(48).size(), QSize(32, 32)); // smaller
        QCOMPARE(appointmentIcon.pixmap(16).size(), QSize(16, 16)); // scaled down
        QCOMPARE(appointmentIcon.pixmap(8).size(), QSize(8, 8)); // scaled down
        QCOMPARE(appointmentIcon.pixmap(16).size(), QSize(16, 16)); // scaled down
    }

    QByteArray ba;
    // write to QByteArray
    {
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        QDataStream stream(&buffer);
        stream << abIcon;
    }

    // read from QByteArray
    {
        QBuffer buffer(&ba);
        buffer.open(QIODevice::ReadOnly);
        QDataStream stream(&buffer);
        QIcon i;
        stream >> i;
        QCOMPARE(i.isNull(), abIcon.isNull());
        QCOMPARE(i.availableSizes(), abIcon.availableSizes());
    }

    // Setting or changing the fallback theme should invalidate earlier lookups.
    // We can only test this if the system doesn't provide an icon, because once
    // we got a valid icon, it will be cached, and even if we proxy to a different
    // engine when a fallback theme is set, the cacheKey of the icon will be the
    // same.
    const QIcon editCut = QIcon::fromTheme("edit-cut");
    if (editCut.isNull()) {
        QIcon::setFallbackThemeName("fallbacktheme");
        QVERIFY(!QIcon::fromTheme("edit-cut").isNull());
    }

    // Make sure setting the theme name clears the state
    QIcon::setThemeName("");
    abIcon = QIcon::fromTheme("address-book-new");
    QCOMPARE_NE(abIcon.isNull(), abIconFromPlatform);

    // Test fallback icon behavior for empty theme names.
    // Can only reliably test this on systems that don't have a
    // named system icon theme.
    QIcon::setThemeName(""); // Reset user-theme
    if (QIcon::themeName().isEmpty()) {
        // Test icon from fallback theme even when theme name is empty
        QIcon::setFallbackThemeName("fallbacktheme");
        QVERIFY(!QIcon::fromTheme("edit-cut").isNull());

        // Test icon from fallback path even when theme name is empty
        fallbackIcon = QIcon::fromTheme("red");
        QVERIFY(!fallbackIcon.isNull());
        QVERIFY(QIcon::hasThemeIcon("red"));
        QCOMPARE(fallbackIcon.availableSizes().size(), 1);
    }

    // Passing a full path to fromTheme is not very useful, but should work anyway
    QIcon fullPathIcon = QIcon::fromTheme(m_pngImageFileName);
    QVERIFY(!fullPathIcon.isNull());

    // Restore to system fallback theme
    QIcon::setFallbackThemeName("");
}

static inline QString findGtkUpdateIconCache()
{
    QString binary = QLatin1String("gtk-update-icon-cache");
#ifdef Q_OS_WIN
    binary += QLatin1String(".exe");
#endif
    return QStandardPaths::findExecutable(binary);
}

void tst_QIcon::fromThemeCache()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));

    QVERIFY(QDir().mkpath(dir.path() + QLatin1String("/testcache/16x16/actions")));
    QVERIFY(QFile(QStringLiteral(":/styles/commonstyle/images/standardbutton-open-16.png"))
        .copy( dir.path() + QLatin1String("/testcache/16x16/actions/button-open.png")));

    {
        QFile index(dir.path() + QLatin1String("/testcache/index.theme"));
        QVERIFY(index.open(QFile::WriteOnly));
        index.write("[Icon Theme]\nDirectories=16x16/actions\n[16x16/actions]\nSize=16\nContext=Actions\nType=Fixed\n");
    }
    QIcon::setThemeSearchPaths(QStringList() << dir.path());
    QIcon::setThemeName("testcache");

    // We just created a theme with that icon, it must exist
    QVERIFY(!QIcon::fromTheme("button-open").isNull());

    QString cacheName = dir.path() + QLatin1String("/testcache/icon-theme.cache");

    // An invalid cache should not prevent lookup
    {
        QFile cacheFile(cacheName);
        QVERIFY(cacheFile.open(QFile::WriteOnly));
        QDataStream(&cacheFile) << quint16(1) << quint16(0) << "invalid corrupted stuff in there\n";
    }
    QIcon::setThemeSearchPaths(QStringList() << dir.path()); // reload themes
    QVERIFY(!QIcon::fromTheme("button-open").isNull());

    // An empty cache should prevent the lookup
    {
        QFile cacheFile(cacheName);
        QVERIFY(cacheFile.open(QFile::WriteOnly));
        QDataStream ds(&cacheFile);
        ds << quint16(1) << quint16(0); // 0: version
        ds << quint32(12) << quint32(20); // 4: hash offset / dir list offset
        ds << quint32(1) << quint32(0xffffffff); // 12: one empty bucket
        ds << quint32(1) << quint32(28); // 20: list with one element
        ds.writeRawData("16x16/actions", sizeof("16x16/actions")); // 28
    }
    QIcon::setThemeSearchPaths(QStringList() << dir.path()); // reload themes
    QVERIFY(QIcon::fromTheme("button-open").isNull()); // The icon was not in the cache, it should not be found

    // Adding an icon should be changing the modification date of one sub directory which should make the cache ignored
    QTest::qWait(1000); // wait enough to have a different modification time in seconds
    QVERIFY(QFile(QStringLiteral(":/styles/commonstyle/images/standardbutton-save-16.png"))
        .copy(dir.path() + QLatin1String("/testcache/16x16/actions/button-save.png")));
    QVERIFY(QFileInfo(cacheName).lastModified(QTimeZone::UTC) < QFileInfo(dir.path() + QLatin1String("/testcache/16x16/actions")).lastModified(QTimeZone::UTC));
    QIcon::setThemeSearchPaths(QStringList() << dir.path()); // reload themes
    QVERIFY(!QIcon::fromTheme("button-open").isNull());

    // Try to run the actual gtk-update-icon-cache and make sure that icons are still found
    const QString gtkUpdateIconCache = findGtkUpdateIconCache();
    if (gtkUpdateIconCache.isEmpty()) {
        QIcon::setThemeSearchPaths(QStringList());
        QSKIP("gtk-update-icon-cache not run (binary not found)");
    }
#if QT_CONFIG(process)
    QProcess process;
    process.start(gtkUpdateIconCache,
                  QStringList() << QStringLiteral("-f") << QStringLiteral("-t") << (dir.path() + QLatin1String("/testcache")));
    QVERIFY2(process.waitForStarted(), qPrintable(QLatin1String("Unable to start: ")
                                                  + gtkUpdateIconCache + QLatin1String(": ")
                                                  + process.errorString()));
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
#endif // QT_CONFIG(process)
    QVERIFY(QFileInfo(cacheName).lastModified(QTimeZone::UTC) >= QFileInfo(dir.path() + QLatin1String("/testcache/16x16/actions")).lastModified(QTimeZone::UTC));
    QIcon::setThemeSearchPaths(QStringList() << dir.path()); // reload themes
    QVERIFY(!QIcon::fromTheme("button-open").isNull());
    QVERIFY(!QIcon::fromTheme("button-open-fallback").isNull());
    QVERIFY(QIcon::fromTheme("notexist-fallback").isNull());
}

void tst_QIcon::fromThemeConstant()
{
    const QIcon icon = QIcon::fromTheme(QIcon::ThemeIcon::EditCut);
}

void tst_QIcon::task223279_inconsistentAddFile()
{
    QIcon icon1;
    icon1.addFile(QLatin1String(":/styles/commonstyle/images/standardbutton-open-16.png"));
    icon1.addFile(QLatin1String("IconThatDoesntExist"), QSize(32, 32));
    QPixmap pm1 = icon1.pixmap(32, 32);

    QIcon icon2;
    icon2.addFile(QLatin1String(":/styles/commonstyle/images/standardbutton-open-16.png"));
    icon2.addFile(QLatin1String("IconThatDoesntExist"));
    QPixmap pm2 = icon1.pixmap(32, 32);

    QCOMPARE(pm1.isNull(), false);
    QCOMPARE(pm1.size(), QSize(16,16));
    QCOMPARE(pm1.isNull(), pm2.isNull());
    QCOMPARE(pm1.size(), pm2.size());
}

Q_IMPORT_PLUGIN(TestIconPlugin)

void tst_QIcon::themeFromPlugin_data()
{
    QTest::addColumn<QString>("themeName");

    QTest::addRow("plugintheme") << "plugintheme";
    QTest::addRow("specialtheme") << "specialTheme"; // deliberately not matching case
}

void tst_QIcon::themeFromPlugin()
{
    QFETCH(const QString, themeName);
    auto restoreTheme = qScopeGuard([oldTheme = QIcon::themeName()]{
        QIcon::setThemeName(oldTheme);
    });

    QIcon icon = QIcon::fromTheme("icon1");
    QVERIFY(icon.isNull());

    QIcon::setThemeName(themeName);

    icon = QIcon::fromTheme("icon1");
    QVERIFY(!icon.isNull());
    QCOMPARE(icon.name(), themeName + "/icon1");
}

QTEST_MAIN(tst_QIcon)
#include "tst_qicon.moc"
