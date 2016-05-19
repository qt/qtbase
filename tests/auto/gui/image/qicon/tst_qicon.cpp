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
#include <QImageReader>
#include <qicon.h>
#include <qiconengine.h>
#include <QtCore/QStandardPaths>

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
    void availableSizes();
    void name();
    void streamAvailableSizes_data();
    void streamAvailableSizes();
    void fromTheme();
    void fromThemeCache();

#ifndef QT_NO_WIDGETS
    void task184901_badCache();
#endif
    void task223279_inconsistentAddFile();

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
    , m_sourceFileName(QFINDTESTDATA(__FILE__))
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

    {
        QPixmap pixmap(source);
        QIcon icon(pixmap);
        QCOMPARE(icon.actualSize(argument), result);
        QCOMPARE(icon.pixmap(argument).size(), result);
    }

    {
        QIcon icon(source);
        QCOMPARE(icon.actualSize(argument), result);
        QCOMPARE(icon.pixmap(argument).size(), result);
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
    QVERIFY(!iconEmptyString.actualSize(QSize(32, 32)).isValid());;

    // test string constructor with non-existing file
    QIcon iconNoFile = QIcon("imagedoesnotexist");
    QVERIFY(!iconNoFile.isNull());
    QVERIFY(!iconNoFile.actualSize(QSize(32, 32)).isValid());

    // test string constructor with non-existing file with suffix
    QIcon iconNoFileSuffix = QIcon("imagedoesnotexist.png");
    QVERIFY(!iconNoFileSuffix.isNull());
    QVERIFY(!iconNoFileSuffix.actualSize(QSize(32, 32)).isValid());

    // test string constructor with existing file but unsupported format
    QIcon iconUnsupportedFormat = QIcon(m_sourceFileName);
    QVERIFY(!iconUnsupportedFormat.isNull());
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
    QCOMPARE(img1, img2);
}

void tst_QIcon::addFile()
{
    QIcon icon;
    icon.addFile(QLatin1String(":/styles/commonstyle/images/standardbutton-open-16.png"));
    icon.addFile(QLatin1String(":/styles/commonstyle/images/standardbutton-open-32.png"));
    icon.addFile(QLatin1String(":/styles/commonstyle/images/standardbutton-open-128.png"));
    icon.addFile(QLatin1String(":/styles/commonstyle/images/standardbutton-save-16.png"), QSize(), QIcon::Selected);
    icon.addFile(QLatin1String(":/styles/commonstyle/images/standardbutton-save-32.png"), QSize(), QIcon::Selected);
    icon.addFile(QLatin1String(":/styles/commonstyle/images/standardbutton-save-128.png"), QSize(), QIcon::Selected);

#ifndef Q_OS_WINCE
    QVERIFY(icon.pixmap(16, QIcon::Normal).toImage() ==
            QPixmap(QLatin1String(":/styles/commonstyle/images/standardbutton-open-16.png")).toImage());
    QVERIFY(icon.pixmap(32, QIcon::Normal).toImage() ==
            QPixmap(QLatin1String(":/styles/commonstyle/images/standardbutton-open-32.png")).toImage());
    QVERIFY(icon.pixmap(128, QIcon::Normal).toImage() ==
            QPixmap(QLatin1String(":/styles/commonstyle/images/standardbutton-open-128.png")).toImage());
    QVERIFY(icon.pixmap(16, QIcon::Selected).toImage() ==
            QPixmap(QLatin1String(":/styles/commonstyle/images/standardbutton-save-16.png")).toImage());
    QVERIFY(icon.pixmap(32, QIcon::Selected).toImage() ==
            QPixmap(QLatin1String(":/styles/commonstyle/images/standardbutton-save-32.png")).toImage());
    QVERIFY(icon.pixmap(128, QIcon::Selected).toImage() ==
            QPixmap(QLatin1String(":/styles/commonstyle/images/standardbutton-save-128.png")).toImage());
#else
    // WinCE only includes the 16x16 images for size reasons
    QVERIFY(icon.pixmap(16, QIcon::Normal).toImage() ==
            QPixmap(QLatin1String(":/styles/commonstyle/images/standardbutton-open-16.png")).toImage());
    QVERIFY(icon.pixmap(16, QIcon::Selected).toImage() ==
            QPixmap(QLatin1String(":/styles/commonstyle/images/standardbutton-save-16.png")).toImage());
#endif
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


static inline bool operator<(const QSize &lhs, const QSize &rhs)
{
    if (lhs.width() < rhs.width())
        return true;
    else if (lhs.width() == lhs.width())
        return lhs.height() < lhs.height();
    return false;
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
    QString firstSearchPath = QLatin1String(":/icons");
    QString secondSearchPath = QLatin1String(":/second_icons");
    QIcon::setThemeSearchPaths(QStringList() << firstSearchPath << secondSearchPath);
    QCOMPARE(QIcon::themeSearchPaths().size(), 2);
    QCOMPARE(firstSearchPath, QIcon::themeSearchPaths()[0]);
    QCOMPARE(secondSearchPath, QIcon::themeSearchPaths()[1]);

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

    // Test non existing icon
    QIcon noIcon = QIcon::fromTheme("broken-icon");
    QVERIFY(noIcon.isNull());
    QVERIFY(!QIcon::hasThemeIcon("broken-icon"));

    // Test non existing icon with fallback
    noIcon = QIcon::fromTheme("broken-icon", abIcon);
    QCOMPARE(noIcon.cacheKey(), abIcon.cacheKey());

    // Test svg-only icon
    noIcon = QIcon::fromTheme("svg-icon", abIcon);
    QVERIFY(!noIcon.availableSizes().isEmpty());

    // Pixmaps should be no larger than the requested size (QTBUG-17953)
    QCOMPARE(appointmentIcon.pixmap(22).size(), QSize(22, 22)); // exact
    QCOMPARE(appointmentIcon.pixmap(32).size(), QSize(32, 32)); // exact
    QCOMPARE(appointmentIcon.pixmap(48).size(), QSize(32, 32)); // smaller
    QCOMPARE(appointmentIcon.pixmap(16).size(), QSize(16, 16)); // scaled down
    QCOMPARE(appointmentIcon.pixmap(8).size(), QSize(8, 8)); // scaled down
    QCOMPARE(appointmentIcon.pixmap(16).size(), QSize(16, 16)); // scaled down

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

    // Make sure setting the theme name clears the state
    QIcon::setThemeName("");
    abIcon = QIcon::fromTheme("address-book-new");
    QVERIFY(abIcon.isNull());

    // Passing a full path to fromTheme is not very useful, but should work anyway
    QIcon fullPathIcon = QIcon::fromTheme(m_pngImageFileName);
    QVERIFY(!fullPathIcon.isNull());
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
    QVERIFY(QFileInfo(cacheName).lastModified() < QFileInfo(dir.path() + QLatin1String("/testcache/16x16/actions")).lastModified());
    QIcon::setThemeSearchPaths(QStringList() << dir.path()); // reload themes
    QVERIFY(!QIcon::fromTheme("button-open").isNull());

    // Try to run the actual gtk-update-icon-cache and make sure that icons are still found
    const QString gtkUpdateIconCache = findGtkUpdateIconCache();
    if (gtkUpdateIconCache.isEmpty()) {
        QIcon::setThemeSearchPaths(QStringList());
        QSKIP("gtk-update-icon-cache not run (binary not found)");
    }
#ifndef QT_NO_PROCESS
    QProcess process;
    process.start(gtkUpdateIconCache,
                  QStringList() << QStringLiteral("-f") << QStringLiteral("-t") << (dir.path() + QLatin1String("/testcache")));
    QVERIFY2(process.waitForStarted(), qPrintable(QLatin1String("Unable to start: ")
                                                  + gtkUpdateIconCache + QLatin1String(": ")
                                                  + process.errorString()));
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
#endif // QT_NO_PROCESS
    QVERIFY(QFileInfo(cacheName).lastModified() >= QFileInfo(dir.path() + QLatin1String("/testcache/16x16/actions")).lastModified());
    QIcon::setThemeSearchPaths(QStringList() << dir.path()); // reload themes
    QVERIFY(!QIcon::fromTheme("button-open").isNull());
    QVERIFY(!QIcon::fromTheme("button-open-fallback").isNull());
    QVERIFY(QIcon::fromTheme("notexist-fallback").isNull());
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


QTEST_MAIN(tst_QIcon)
#include "tst_qicon.moc"
