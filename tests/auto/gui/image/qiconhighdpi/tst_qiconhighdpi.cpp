// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <qicon.h>

class tst_QIconHighDpi : public QObject
{
    Q_OBJECT
public:
    tst_QIconHighDpi();

private slots:
    void initTestCase();
    void fromTheme_data();
    void fromTheme();
    void addPixmap_data();
    void addPixmap();
    void ninePatch();
};

tst_QIconHighDpi::tst_QIconHighDpi()
{
}

void tst_QIconHighDpi::initTestCase()
{
}

void tst_QIconHighDpi::fromTheme_data()
{
    QTest::addColumn<int>("requestedSize");
    QTest::addColumn<qreal>("requestedDpr");
    QTest::addColumn<int>("expectedSize");
    QTest::addColumn<qreal>("expectedDpr");

    // The pixmaps that we have available can be found in tst_qiconhighdpi.qrc.
    // Currently we only have @1 and @2 icons available, which limits the expected
    // devicePixelRatio when testing at e.g. devicePixelRatio 3.

    // We have an @2 high DPI version of the 22x22 size of this icon.
    QTest::newRow("22x22,dpr=1") << 22 << 1.0 << 22 << 1.0;
    QTest::newRow("22x22,dpr=2") << 22 << 2.0 << 44 << 2.0;
    QTest::newRow("22x22,dpr=3") << 22 << 3.0 << 44 << 2.0;

    // We don't have a high DPI version of the 16x16 size of this icon,
    // so directoryMatchesSize() will return false for all directories.
    // directorySizeDistance() is then called to find the best match.
    // The table below illustrates the results for our available images at various DPRs:
    // Available size | Available scale | Requested size | Requested scale | Distance
    //        22      *        2        -       16       *        1        =    28
    //        22      *        1        -       16       *        1        =     6
    //        16      *        1        -       16       *        1        =     0 < (16x16)
    // Available size | Available scale | Requested size | Requested scale | Distance
    //        22      *        2        -       16       *        2        =    12
    //        22      *        1        -       16       *        2        =    10 < (22x22)
    //        16      *        1        -       16       *        2        =    16
    // Available size | Available scale | Requested size | Requested scale | Distance
    //        22      *        2        -       16       *        3        =     4 < (22x22@2)
    //        22      *        1        -       16       *        3        =    26
    //        16      *        1        -       16       *        3        =    32
    // Both of these functions are implementations of the freedesktop icon theme spec,
    // which dictates that if there is no matching scale, directorySizeDistance() determines
    // the winner, regardless of whether or not the scale is too low for the requested scale.
    QTest::newRow("16x16,dpr=1") << 16 << 1.0 << 16 << 1.0;
    // PixmapEntry::pixmap() will only downscale the pixmap if actualSize.width() > size.width().
    // In this case, 22 > 32 is false, so a 22x22 pixmap is returned.
    QTest::newRow("16x16,dpr=2") << 16 << 2.0 << 22 << 1.375;
    QTest::newRow("16x16,dpr=3") << 16 << 3.0 << 44 << 2.75;

    // We don't have an 8x8 size of this icon, so:
    // Available size | Available scale | Requested size | Requested scale | Distance
    //        22      *        2        -       8        *        1        =    36
    //        22      *        1        -       8        *        1        =    14
    //        16      *        1        -       8        *        1        =     8 < (16x16)
    // Available size | Available scale | Requested size | Requested scale | Distance
    //        22      *        2        -       8        *        2        =    28
    //        22      *        1        -       8        *        2        =     6
    //        16      *        1        -       8        *        2        =     0 < (16x16)
    // Available size | Available scale | Requested size | Requested scale | Distance
    //        22      *        2        -       8        *        3        =    20
    //        22      *        1        -       8        *        3        =     2 < (22x22)
    //        16      *        1        -       8        *        3        =     8
    QTest::newRow("8x8,dpr=1") << 8 << 1.0 << 8 << 1.0;
    QTest::newRow("8x8,dpr=2") << 8 << 2.0 << 16 << 2.0;
    QTest::newRow("8x8,dpr=3") << 8 << 3.0 << 22 << 2.75;

    // We don't have a 44x44 size of this icon, so:
    // Available size | Available scale | Requested size | Requested scale | Distance
    //        22      *        2        -       44       *        1        =     0 < (22x22@2)
    //        22      *        1        -       44       *        1        =    22
    //        16      *        1        -       44       *        1        =    28
    // Available size | Available scale | Requested size | Requested scale | Distance
    //        22      *        2        -       44       *        2        =    44 < (22x22@2)
    //        22      *        1        -       44       *        2        =    66
    //        16      *        1        -       44       *        2        =    72
    // Available size | Available scale | Requested size | Requested scale | Distance
    //        22      *        2        -       44       *        3        =    88 < (22x22@2)
    //        22      *        1        -       44       *        3        =   110
    //        16      *        1        -       44       *        3        =   116
    QTest::newRow("44x44,dpr=1") << 44 << 1.0 << 44 << 1.0;
    QTest::newRow("44x44,dpr=2") << 44 << 2.0 << 44 << 1.0;
    QTest::newRow("44x44,dpr=3") << 44 << 3.0 << 44 << 1.0;

    // We don't have a 20x20 size of this icon, so:
    // Available size | Available scale | Requested size | Requested scale | Distance
    //        22      *        2        -       20       *        1        =    24
    //        22      *        1        -       20       *        1        =     2 < (22x22)
    //        16      *        1        -       20       *        1        =     4
    // Available size | Available scale | Requested size | Requested scale | Distance
    //        22      *        2        -       20       *        2        =     4 < (22x22@2)
    //        22      *        1        -       20       *        2        =    18
    //        16      *        1        -       20       *        2        =    24
    // Available size | Available scale | Requested size | Requested scale | Distance
    //        22      *        2        -       20       *        3        =    16 < (22x22@2)
    //        22      *        1        -       20       *        3        =    38
    //        16      *        1        -       20       *        3        =    44
    QTest::newRow("20x20,dpr=1") << 20 << 1.0 << 20 << 1.0;
    // PixmapEntry::pixmap() will only downscale the pixmap if actualSize.width() > size.width().
    // In this case, 44 > 40 is true, so the 44x44 pixmap is downscaled to 40x40.
    QTest::newRow("20x20,dpr=2") << 20 << 2.0 << 40 << 2.0;
    QTest::newRow("20x20,dpr=3") << 20 << 3.0 << 44 << 2.2;
}

void tst_QIconHighDpi::fromTheme()
{
    QFETCH(int, requestedSize);
    QFETCH(qreal, requestedDpr);
    QFETCH(int, expectedSize);
    QFETCH(qreal, expectedDpr);

    QString searchPath = QLatin1String(":/icons");
    QIcon::setThemeSearchPaths(QStringList() << searchPath);
    QCOMPARE(QIcon::themeSearchPaths().size(), 1);
    QCOMPARE(searchPath, QIcon::themeSearchPaths()[0]);

    QString themeName("testtheme");
    QIcon::setThemeName(themeName);
    QCOMPARE(QIcon::themeName(), themeName);

    QIcon appointmentIcon = QIcon::fromTheme("appointment-new");
    QVERIFY(!appointmentIcon.isNull());
    QVERIFY(!appointmentIcon.availableSizes(QIcon::Normal, QIcon::Off).isEmpty());
    QVERIFY(appointmentIcon.availableSizes().contains(QSize(16, 16)));
    QVERIFY(appointmentIcon.availableSizes().contains(QSize(22, 22)));

    const QPixmap pixmap = appointmentIcon.pixmap(QSize(requestedSize, requestedSize), requestedDpr);
    QCOMPARE(pixmap.size(), QSize(expectedSize, expectedSize));
    // We should get the high DPI version of an image if it exists in the correct directory.
    QCOMPARE(pixmap.devicePixelRatio(), expectedDpr);
}

void tst_QIconHighDpi::addPixmap_data()
{
    // fromTheme() and addPixmap() test the QIconLoaderEngine and QPixmapIconEngine
    // QIcon backend, repsectively. We want these to have identical behavior and
    // re-use the same test data here.
    fromTheme_data();
}

void tst_QIconHighDpi::addPixmap()
{
    QFETCH(int, requestedSize);
    QFETCH(qreal, requestedDpr);
    QFETCH(int, expectedSize);
    QFETCH(qreal, expectedDpr);

    QIcon icon;

    // manual pixmap adder for full control of devicePixelRatio
    auto addPixmapWithDpr = [&icon](const QString &path, qreal dpr) {
        QImage image(path);
        image.setDevicePixelRatio(dpr);
        icon.addPixmap(QPixmap::fromImage(image));
    };

    addPixmapWithDpr(":icons/testtheme/16x16/actions/appointment-new.png", 1);
    addPixmapWithDpr(":icons/testtheme/22x22/actions/appointment-new.png", 1);
    addPixmapWithDpr(":icons/testtheme/22x22@2/actions/appointment-new.png", 2);

    QVERIFY(icon.availableSizes().contains(QSize(16, 16)));
    QVERIFY(icon.availableSizes().contains(QSize(22, 22)));

    if (qstrcmp(QTest::currentDataTag(), "16x16,dpr=2") == 0)
        QSKIP("broken corner case"); // expect 22x22, get 32x32
    if (qstrcmp(QTest::currentDataTag(), "44x44,dpr=1") == 0)
        QSKIP("broken corner case"); // expect 44x44, get 22x22
    if (qstrcmp(QTest::currentDataTag(), "8x8,dpr=3") == 0)
        QSKIP("broken corner case");// expect 22x22, get 24x24

    const QPixmap pixmap = icon.pixmap(QSize(requestedSize, requestedSize), requestedDpr);
    QCOMPARE(pixmap.size(), QSize(expectedSize, expectedSize));
    QCOMPARE(pixmap.devicePixelRatio(), expectedDpr);
}

void tst_QIconHighDpi::ninePatch()
{
    const QIcon icon(":/icons/misc/button.9.png");
    const int dpr = qCeil(qApp->devicePixelRatio());

    switch (dpr) {
    case 1:
        QCOMPARE(icon.availableSizes().size(), 1);
        QCOMPARE(icon.availableSizes().at(0), QSize(42, 42));
        break;
    case 2:
        QCOMPARE(icon.availableSizes().size(), 2);
        QCOMPARE(icon.availableSizes().at(0), QSize(42, 42));
        QCOMPARE(icon.availableSizes().at(1), QSize(82, 82));
        break;
    }
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    Q_UNUSED(app);
    tst_QIconHighDpi test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "tst_qiconhighdpi.moc"
