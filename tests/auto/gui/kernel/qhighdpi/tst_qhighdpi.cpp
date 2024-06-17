// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <private/qhighdpiscaling_p.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qplatformnativeinterface.h>

#include <QTest>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QStringView>
#include <QSignalSpy>

Q_LOGGING_CATEGORY(lcTests, "qt.gui.tests")

class tst_QHighDpi: public QObject
{
    Q_OBJECT
private: // helpers
    QJsonArray createStandardScreens(const QList<qreal> &dpiValues);
    QGuiApplication *createOffscreenApplication(const QByteArray &jsonConfig);
    QGuiApplication *createStandardOffscreenApp(const QList<qreal> &dpiValues);
    QGuiApplication *createStandardOffscreenApp(const QJsonArray &screens);
    static void standardScreenDpiTestData();

    static void setOffscreenConfiguration(const QJsonObject &configuration);
    static QJsonObject offscreenConfiguration();

private slots:
    void initTestCase();
    void cleanup();
    void qhighdpiscaling_data();
    void qhighdpiscaling();
    void minimumDpr();
    void noscreens();
    void screenDpiAndDpr_data();
    void screenDpiAndDpr();
    void screenDpiChange();
    void screenDpiChangeWithWindow();
    void environment_QT_SCALE_FACTOR();
    void environment_QT_SCREEN_SCALE_FACTORS_data();
    void environment_QT_SCREEN_SCALE_FACTORS();
    void environment_QT_USE_PHYSICAL_DPI();
    void environment_QT_SCALE_FACTOR_ROUNDING_POLICY();
    void application_setScaleFactorRoundingPolicy();
    void screenAt_data();
    void screenAt();
    void screenGeometry_data();
    void screenGeometry();
    void windowGeometry_data();
    void windowGeometry();
    void spanningWindows_data();
    void spanningWindows();
    void mouseEvents_data();
    void mouseEvents();
    void mouseVelocity();
    void mouseVelocity_data();
    void setCursor();
    void setCursor_data();
    void setGlobalFactorEmits();
    void setScreenFactorEmits();
};

/// Offscreen platform plugin test setup
const int standardScreenWidth = 640;
const int standardScreenHeight = 480;
const int standardBaseDpi = 96;
const int standardScreenCount = 3;

QJsonArray tst_QHighDpi::createStandardScreens(const QList<qreal> &dpiValues)
{
    Q_ASSERT(dpiValues.size() == standardScreenCount);

    // Create row of three screens: screen#0 screen#1 screen#2
    return QJsonArray {
        QJsonObject {
            {"name", "screen#0"},
            {"x", -standardScreenWidth},
            {"y", -10},
            {"width", standardScreenWidth},
            {"height", standardScreenHeight},
            {"logicalDpi", dpiValues[0]},
            {"logicalBaseDpi", standardBaseDpi},
            {"dpr", 1}
        },
        QJsonObject {
            {"name", "screen#1"},
            {"x", 0},
            {"y", 0},
            {"width", standardScreenWidth},
            {"height", standardScreenHeight},
            {"logicalDpi", dpiValues[1]},
            {"logicalBaseDpi", standardBaseDpi},
            {"dpr", 1}
        },
        QJsonObject {
            {"name", "screen#2"},
            {"x", standardScreenWidth},
            {"y", 10},
            {"width", standardScreenWidth},
            {"height", standardScreenHeight},
            {"logicalDpi", dpiValues[2]},
            {"logicalBaseDpi", standardBaseDpi},
            {"dpr", 1}
        }
    };
}

QGuiApplication *tst_QHighDpi::createOffscreenApplication(const QByteArray &jsonConfig)
{
    // Write offscreen platform config file
    QFile configFile(QLatin1String("qt-offscreen-test-config.json"));
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    configFile.resize(0); // truncate
    if (configFile.write(jsonConfig) == -1)
        qFatal("Could not write config file: %s", qPrintable(configFile.errorString()));
    configFile.close();

    // Create QGuiApplication which loads the offscreen platform plugin
    // Note that argc and argv need to stay valid for the duration of the app lifetime,
    // and may be used at any point. The config file used at app startup only.
    static int argc;
    argc = 3;
    static char *argv[3];
    static QByteArray binaryNameArg = QByteArray("tst_qguiapplication");
    argv[0] = binaryNameArg.data();
    static QByteArray platformArg = QByteArray("-platform");
    argv[1] = platformArg.data();
    static QByteArray offscreenAndFileArg;
    offscreenAndFileArg = QByteArray("offscreen:configfile=") + configFile.fileName().toUtf8();
    argv[2] = offscreenAndFileArg.data();

    QGuiApplication *app = new QGuiApplication(argc, argv);
    configFile.remove(); // config file is needed during QGuiApplication construction only.
    return app;
}

QGuiApplication *tst_QHighDpi::createStandardOffscreenApp(const QList<qreal> &dpiValues)
{
    QJsonArray screens = createStandardScreens(dpiValues);
    return createStandardOffscreenApp(screens);
}

QGuiApplication *tst_QHighDpi::createStandardOffscreenApp(const QJsonArray &screens)
{
    QJsonObject config {
        {"synchronousWindowSystemEvents", true},
        {"windowFrameMargins", false},
        {"screens" , screens},
    };
    return createOffscreenApplication(QJsonDocument(config).toJson());
}

void tst_QHighDpi::initTestCase()
{
    QDir::setCurrent(QDir::tempPath());
}

/// Auto test begins

void tst_QHighDpi::standardScreenDpiTestData()
{
    // We run each test under different DPI configurations, each with three screens:
    QTest::addColumn<QList<qreal>>("dpiValues");
    // Standard-DPI sanity check
    QTest::newRow("96") << QList<qreal> { 96, 96, 96 };
    // 2x high DPI
    QTest::newRow("192") << QList<qreal> { 192, 192, 192 };
    // Mixed desktop DPI (1.5x, 1.75x, 2x)
    QTest::newRow("144-168-192") << QList<qreal> { 144, 168, 192 };
    // Densities from Android's DisplayMetrics docs, normalized to base 96 DPI
    QTest::newRow("240-252-360") << QList<qreal> { 400./160 * 96, 420./160 * 96, 600./160 * 96 };
}

void tst_QHighDpi::setOffscreenConfiguration(const QJsonObject &configuration)
{
    Q_ASSERT(qApp->platformName() == QLatin1String("offscreen"));
    QPlatformNativeInterface *platformNativeInterface = qApp->platformNativeInterface();
    auto setConfiguration = reinterpret_cast<void (*)(QJsonObject, QPlatformNativeInterface *)>(
        platformNativeInterface->nativeResourceForIntegration("setConfiguration"));
    setConfiguration(configuration, platformNativeInterface);
}

QJsonObject tst_QHighDpi::offscreenConfiguration()
{
    Q_ASSERT(qApp->platformName() == QLatin1String("offscreen"));
    QPlatformNativeInterface *platformNativeInterface = qApp->platformNativeInterface();
    auto getConfiguration = reinterpret_cast<QJsonObject (*)(QPlatformNativeInterface *)>(
        platformNativeInterface->nativeResourceForIntegration("configuration"));
    return getConfiguration(platformNativeInterface);
}

void tst_QHighDpi::cleanup()
{
    // Some test functions set environment variables. Unset them here,
    // in order to avoid getting confusing follow-on errors on test failures.
    qunsetenv("QT_SCALE_FACTOR");
    qunsetenv("QT_SCREEN_SCALE_FACTORS");
    qunsetenv("QT_USE_PHYSICAL_DPI");
    qunsetenv("QT_SCALE_FACTOR_ROUNDING_POLICY");
}

void tst_QHighDpi::qhighdpiscaling_data()
{
    standardScreenDpiTestData();
}

// Tests the QHighDpiScaling API directly
void tst_QHighDpi::qhighdpiscaling()
{
    QFETCH(QList<qreal>, dpiValues);
    std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(dpiValues));

    QHighDpiScaling::setGlobalFactor(2);

    // Verfy that QHighDpiScaling::factor() does not crash on nullptr contexts.
    QScreen *screenContext = nullptr;
    QVERIFY(QHighDpiScaling::factor(screenContext) >= 0);
    QPlatformScreen *platformScreenContext = nullptr;
    QVERIFY(QHighDpiScaling::factor(platformScreenContext) >= 0);
    QWindow *windowContext = nullptr;
    QVERIFY(QHighDpiScaling::factor(windowContext) >= 0);
    QHighDpiScaling::setGlobalFactor(1);
}

void tst_QHighDpi::screenDpiAndDpr_data()
{
    standardScreenDpiTestData();
}

void tst_QHighDpi::screenDpiAndDpr()
{
    QFETCH(QList<qreal>, dpiValues);

    std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(dpiValues));
    int i = 0;
    for (QScreen *screen : app->screens()) {
        qreal dpi = dpiValues[i++];

        // verify that the devicePixelRatio equation holds: DPR = DPI / BaseDPI
        QCOMPARE(screen->devicePixelRatio(), dpi / standardBaseDpi);
        QCOMPARE(screen->logicalDotsPerInch(), dpi / screen->devicePixelRatio());

        QWindow window(screen);
        QCOMPARE(window.devicePixelRatio(), screen->devicePixelRatio());
        window.setGeometry(QRect(screen->geometry().center(), QSize(10, 10)));
        window.create();
        QCOMPARE(window.devicePixelRatio(), screen->devicePixelRatio());
    }
}

void tst_QHighDpi::screenDpiChange()
{
    QList<qreal> dpiValues = { 96, 96, 96};
    std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(dpiValues));

    QCOMPARE(app->devicePixelRatio(), 1);

    // Set new DPI
    int newDpi = 192;
    QJsonValue config = offscreenConfiguration();
    // API defect until Qt 7, so go indirectly via CBOR
    QCborMap map = QCborMap::fromJsonObject(config.toObject());
    map[QLatin1String("screens")][0][QLatin1String("logicalDpi")] = newDpi;
    map[QLatin1String("screens")][1][QLatin1String("logicalDpi")] = newDpi;
    map[QLatin1String("screens")][2][QLatin1String("logicalDpi")] = newDpi;
    setOffscreenConfiguration(map.toJsonObject());

    // TODO check events

    // Verify that the new DPI is in use
    for (QScreen *screen : app->screens()) {
        QCOMPARE(screen->devicePixelRatio(), newDpi / standardBaseDpi);
        QCOMPARE(screen->logicalDotsPerInch(), newDpi / screen->devicePixelRatio());

        QWindow window(screen);
        QCOMPARE(window.devicePixelRatio(), screen->devicePixelRatio());
        window.create();
        QCOMPARE(window.devicePixelRatio(), screen->devicePixelRatio());
    }
    QCOMPARE(app->devicePixelRatio(), newDpi / standardBaseDpi);
}

void tst_QHighDpi::screenDpiChangeWithWindow()
{
    QList<qreal> dpiValues = { 96, 192, 288 };
    std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(dpiValues));

    // Create windows for screens
    QList<QScreen *> screens = app->screens();
    QList<QWindow *> windows;
    for (int i = 0; i < screens.count(); ++i) {
        QScreen *screen = screens[i];
        QWindow *window = new QWindow();
        windows.append(window);
        window->setGeometry(QRect(screen->geometry().center(), QSize(10, 10)));
        window->create();
        QCOMPARE(window->devicePixelRatio(), dpiValues[i] / standardBaseDpi);
    }

    // Change screen DPI
    QList<qreal> newDpiValues = { 288, 192, 96 };
    QJsonValue config = offscreenConfiguration();
    QCborMap map = QCborMap::fromJsonObject(config.toObject());
    for (int i = 0; i < screens.count(); ++i) {
        map[QLatin1String("screens")][i][QLatin1String("logicalDpi")] = newDpiValues[i];
    }
    setOffscreenConfiguration(map.toJsonObject());

    // Verify that window DPR changes on Screen DPI change.
    for (int i = 0; i < screens.count(); ++i) {
        QWindow *window = windows[i];
        QCOMPARE(window->devicePixelRatio(), newDpiValues[i] / standardBaseDpi);
    }
}

void tst_QHighDpi::environment_QT_SCALE_FACTOR()
{
    qreal factor = 3.1415;
    qputenv("QT_SCALE_FACTOR", std::to_string(factor));

    QList<qreal> dpiValues { 96, 144, 192 };
    std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(dpiValues));
    int i = 0;
    for (QScreen *screen : app->screens()) {
        // Verify that QT_SCALE_FACTOR applies as a multiplicative factor.
        qreal expextedDpr = (dpiValues[i] / standardBaseDpi) * factor;
        ++i;
        QCOMPARE(screen->devicePixelRatio(), expextedDpr);
        QCOMPARE(screen->logicalDotsPerInch(), 96);
        QWindow window(screen);
        QCOMPARE(window.devicePixelRatio(), expextedDpr);
    }
}

void tst_QHighDpi::environment_QT_SCREEN_SCALE_FACTORS_data()
{
    QTest::addColumn<QList<qreal>>("platformScreenDpi"); // The to-be-overridden values
    QTest::addColumn<QByteArray>("environment");
    QTest::addColumn<QList<qreal>>("expectedDprValues");

    QList<qreal> platformScreenDpi { 192, 216, 240 };
    QList<qreal> fromPlatformScreenDpr { 2, 2.25, 2.5 };
    QList<qreal> fromEnvironmentDpr { 1, 1.5, 2 };

    // Correct env. variable values.
    QTest::newRow("list") << platformScreenDpi << QByteArray("1;1.5;2") << fromEnvironmentDpr;
    QTest::newRow("names") << platformScreenDpi << QByteArray("screen#1=1.5;screen#0=1;screen#2=2") << fromEnvironmentDpr;

    // Various broken env. variable values. Should not crash,
    // and should not change the DPR.
    QTest::newRow("empty") << platformScreenDpi << QByteArray("") << fromPlatformScreenDpr;
    QTest::newRow("bogus-1") << platformScreenDpi << QByteArray("foo=bar") << fromPlatformScreenDpr;
    QTest::newRow("bogus-2") << platformScreenDpi << QByteArray("fo0==2;;=;==;=3") << fromPlatformScreenDpr;
}

void tst_QHighDpi::environment_QT_SCREEN_SCALE_FACTORS()
{
    QFETCH(QList<qreal>, platformScreenDpi);
    QFETCH(QByteArray, environment);
    QFETCH(QList<qreal>, expectedDprValues);

    qputenv("QT_SCREEN_SCALE_FACTORS", environment);

    // Verify that setting QT_SCREEN_SCALE_FACTORS overrides the from-platform-screen-DPI DPR.
    {
        std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(platformScreenDpi));
        int i = 0;
        for (QScreen *screen : app->screens()) {
            qreal expextedDpr = expectedDprValues[i];
            ++i;
            QCOMPARE(screen->devicePixelRatio(), expextedDpr);
            QCOMPARE(screen->logicalDotsPerInch(), 96);
            QWindow window(screen);
            QCOMPARE(window.devicePixelRatio(), expextedDpr);
        }
    }

    // Verify that setHighDpiScaleFactorRoundingPolicy applies to QT_SCREEN_SCALE_FACTORS as well
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Round);
    {
        std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(platformScreenDpi));
        int i = 0;
        for (QScreen *screen : app->screens()) {
            qreal expectedRounderDpr = qRound(expectedDprValues[i++]);
            qreal windowDpr = QWindow(screen).devicePixelRatio();
            QCOMPARE(windowDpr, expectedRounderDpr);
        }
    }
}

void tst_QHighDpi::environment_QT_USE_PHYSICAL_DPI()
{
    qputenv("QT_USE_PHYSICAL_DPI", "1");

    QList<qreal> dpiValues { 96, 144, 192 };
    std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(dpiValues));

    // Verify that the device pixel ratio is computed as physicalDpi / baseDpi.
    // (which in practice uses physicalSize since this is what QPlatformScreen provides)

    // The default QPlatformScreen::physicalSize() implementation (which QOffscreenScreen
    // currerently uses) assumes a default DPI of 100 and calculates a fake physical size
    // based on that value. Use DPI 100 here as well: if you have changed the default value
    // in QPlatformScreen and get a test failure then update the value below.
    const qreal platformScreenDefualtDpi = 100;
    qreal expextedDpr = (platformScreenDefualtDpi / qreal(standardBaseDpi));

    for (QScreen *screen : app->screens()) {
        QCOMPARE(screen->devicePixelRatio(), expextedDpr);
        QCOMPARE(screen->logicalDotsPerInch(), 96);
        QWindow window(screen);
        QCOMPARE(window.devicePixelRatio(), expextedDpr);
    }
}

void tst_QHighDpi::environment_QT_SCALE_FACTOR_ROUNDING_POLICY()
{
    QList<qreal> dpiValues { 96, 144, 192 };

    qputenv("QT_SCALE_FACTOR_ROUNDING_POLICY", "PassThrough");
    {
        std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(dpiValues));
        for (int i = 0; i < dpiValues.size(); ++i)
            QCOMPARE(app->screens()[i]->devicePixelRatio(), dpiValues[i] / qreal(96));
    }

    qputenv("QT_SCALE_FACTOR_ROUNDING_POLICY", "Round");
    {
        std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(dpiValues));
        for (int i = 0; i < dpiValues.size(); ++i)
            QCOMPARE(app->screens()[i]->devicePixelRatio(), qRound(dpiValues[i] / qreal(96)));
    }

    qunsetenv("QT_SCALE_FACTOR_ROUNDING_POLICY");
    {
        std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(dpiValues));
        for (int i = 0; i < dpiValues.size(); ++i)
            QCOMPARE(app->screens()[i]->devicePixelRatio(), dpiValues[i] / qreal(96));
    }
}

void tst_QHighDpi::application_setScaleFactorRoundingPolicy()
{
    QList<qreal> dpiValues { 96, 144, 192 };
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Round);
    {
        std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(dpiValues));
        for (int i = 0; i < dpiValues.size(); ++i)
            QCOMPARE(app->screens()[i]->devicePixelRatio(), qRound(dpiValues[i] / qreal(96)));
    }

    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    {
        std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(dpiValues));
        for (int i = 0; i < dpiValues.size(); ++i)
            QCOMPARE(app->screens()[i]->devicePixelRatio(), dpiValues[i] / qreal(96));
    }

    // Verify that environment overrides app setting
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Round);
    qputenv("QT_SCALE_FACTOR_ROUNDING_POLICY", "PassThrough");
    {
        std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(dpiValues));
        for (int i = 0; i < dpiValues.size(); ++i)
            QCOMPARE(app->screens()[i]->devicePixelRatio(), dpiValues[i] / qreal(96));
    }
}

void tst_QHighDpi::minimumDpr()
{
    QList<qreal> dpiValues { 40, 60, 95 };
    std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(dpiValues));
    for (QScreen *screen : app->screens()) {
        // Qt does not currently support DPR values < 1. Make sure
        // the minimum DPR value is 1, also when the screen reports
        // a low DPI.
        QCOMPARE(screen->devicePixelRatio(), 1);
        QWindow window(screen);
        QCOMPARE(window.devicePixelRatio(), 1);
    }
}

QT_BEGIN_NAMESPACE
extern int qt_defaultDpiX();
extern int qt_defaultDpiY();
QT_END_NAMESPACE

void tst_QHighDpi::noscreens()
{
    // Create application object with a no-screens configuration (should not crash)
    QJsonArray noScreens;
    std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(noScreens));

    QCOMPARE(qApp->devicePixelRatio(), 1);

    // Test calling qt_defaultDpiX/Y: These may be called early during QGuiApplication
    // initialization, before the platform plugin has created screen objects. They
    // should then 1) not crash and 2) return some default value.
    QCOMPARE(qt_defaultDpiX(), qt_defaultDpiY());
}

void tst_QHighDpi::screenAt_data()
{
    standardScreenDpiTestData();
}

void tst_QHighDpi::screenAt()
{
    QFETCH(QList<qreal>, dpiValues);
    std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(dpiValues));

    QCOMPARE(app->screens().size(), standardScreenCount); // standard setup

    // Verify that screenAt() returns the correct or no screen for various points,
    // for all screens.
    int i = 0;
    for (QScreen *screen : app->screens()) {
        qreal dpi =  dpiValues[i++];

        // veryfy virtualSiblings and that AA_EnableHighDpiScaling is active
        QCOMPARE(screen->virtualSiblings().size(), standardScreenCount);
        QCOMPARE(screen->geometry().size(), QSize(standardScreenWidth, standardScreenHeight) * (96.0 / dpi));

        // test points on screen
        QCOMPARE(app->screenAt(screen->geometry().center()), screen);
        QCOMPARE(app->screenAt(screen->geometry().topLeft()), screen);
        QCOMPARE(app->screenAt(screen->geometry().bottomRight()), screen);

        // test points off  screen
        QCOMPARE(app->screenAt(screen->geometry().center() + QPoint(0, -1000)), nullptr);
        QCOMPARE(app->screenAt(screen->geometry().topLeft() + QPoint(0, -1)), nullptr);
        QCOMPARE(app->screenAt(screen->geometry().bottomRight() + QPoint(0, +1)), nullptr);

        // check the "gaps" created by Qt::AA_EnableHighDpiScaling: no screen there
        if (dpi > 96) {
            QCOMPARE(app->screenAt(screen->geometry().topLeft() + QPoint(-1, 0)), nullptr);
            QCOMPARE(app->screenAt(screen->geometry().bottomRight() + QPoint(1, 0)), nullptr);
        }
    }
}

void tst_QHighDpi::screenGeometry_data()
{
    standardScreenDpiTestData();
}

void tst_QHighDpi::screenGeometry()
{
    QFETCH(QList<qreal>, dpiValues);
    std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(dpiValues));

    for (QScreen *screen : app->screens()) {
        QRect geometry = screen->geometry();
        QPoint onScreen = geometry.topLeft() + QPoint(10, 10);
        QPoint onScreenNative = QHighDpi::toNativePixels(onScreen, screen);
        QPoint onScreenBack = QHighDpi::fromNativePixels(onScreenNative, screen);

        QCOMPARE(onScreen, onScreenBack);

        QPoint offScreen = geometry.topLeft() - QPoint(10, 10);
        QPoint offScreenNative = QHighDpi::toNativePixels(offScreen, screen);
        QPoint offScreenBack = QHighDpi::fromNativePixels(offScreenNative, screen);
        QCOMPARE(offScreenBack, offScreenBack);
    }
}

void tst_QHighDpi::windowGeometry_data()
{
    standardScreenDpiTestData();
}

void tst_QHighDpi::windowGeometry()
{
    QFETCH(QList<qreal>, dpiValues);
    std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(dpiValues));

    auto testWindow = [&app](QWindow *window, QScreen *expectedScreen, QPoint expectedPosition, QSize expectedSize) {

        // Is the window correctly sized and on the correct screen?
        QCOMPARE(window->size(), expectedSize);
        QCOMPARE(window->position(), expectedPosition);
        QCOMPARE(window->screen(), expectedScreen);
        QCOMPARE(app->screenAt(window->mapToGlobal(QPoint(0, 0))), expectedScreen);

        // Round-trip coordinates local->global->local, which should return the starting
        // coordinates, also for coordinates outside the window (and screen)
        auto globalRoundtrip = [](QWindow *window, QPoint pos) {
            QCOMPARE(window->mapFromGlobal(window->mapToGlobal(pos)), pos);
        };
        globalRoundtrip(window, QPoint(10, 10)); // window-interior
        globalRoundtrip(window, QPoint(-5, -5)); // outside window, on same screen
        globalRoundtrip(window, QPoint(standardScreenWidth *2, standardScreenHeight * 2)); // Outside window, outside all screens
        globalRoundtrip(window, QPoint(0, -standardScreenWidth)); // Outside window, on neighbor screen

        // Round-trip float coordinates
        auto globalRoundtripF = [](QWindow *window, QPointF pos) {
            QCOMPARE(window->mapFromGlobal(window->mapToGlobal(pos)), pos);
        };

        globalRoundtripF(window, QPointF(10, 10)); // window-interior
        globalRoundtripF(window, QPointF(10.1, 10.1));
        globalRoundtripF(window, QPointF(10.5, 10.5));
        globalRoundtripF(window, QPointF(10.9, 10.9));
        globalRoundtripF(window, QPointF(-5.5, -5.5)); // outside window, on same screen
        globalRoundtripF(window, QPointF(standardScreenWidth * 2.1, standardScreenHeight * 2.1)); // Outside window, outside all screens
        globalRoundtripF(window, QPointF(0.5, -standardScreenWidth)); // Outside window, on neighbor screen
    };

    // verify window geometry for top-level and child windows on all screens
    for (QScreen *screen : app->screens()) {
        QWindow topLevelWindow;
        QSize topLevelSize(40, 40);
        QPoint topLevelPosition(screen->geometry().center());
        topLevelWindow.resize(topLevelSize);
        topLevelWindow.setPosition(topLevelPosition);
        topLevelWindow.show();
        testWindow(&topLevelWindow, screen, topLevelPosition, topLevelSize);

        QWindow childWindow(&topLevelWindow);
        QSize childSize(20, 20);
        QPoint childPosition(10, 10);
        childWindow.resize(childSize);
        childWindow.setPosition(childPosition);
        childWindow.show();
        testWindow(&childWindow, screen, childPosition, childSize);
    }
}

void tst_QHighDpi::spanningWindows_data()
{
    standardScreenDpiTestData();
}

void tst_QHighDpi::spanningWindows()
{
    QFETCH(QList<qreal>, dpiValues);
    std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(dpiValues));

    QPoint screen0Center = app->screens()[0]->geometry().center();
    int screenWidth = app->screens()[0]->geometry().width();

    // Create window spanning screen 0 and screen 1
    QWindow window;
    QRect windowGeometry = QRect(screen0Center, QSize(screenWidth - 10, 20));
    windowGeometry.adjust(0, 0, -10, 0); // Make sure the center point is on screen 0
    window.setGeometry(windowGeometry);
    window.show();
    QCOMPARE(window.geometry(), windowGeometry);

    // Device independent screen space may be non-contiguous, in which case global
    // window geometry behaves non-intuitivly when a window spans multiple screens:
    //   - The main screen for the window is defined by the windowing system
    //     (usually the screen with most window coverage), and is reflected
    //     by QWindow::screen()
    //   - screen coordinate linear math does not work for points on the window
    //     extending beyond the main screen - these may be on a different screen
    //     with a non-linear coordinate offset.
    //
    // Local window geometry works (mostly) as before:
    //   - QWindow::mapToGlobal() can map any window-local coordinate to the correct
    //     global coordinate and screen, as long as the coordinate is on the window.
    //   - QWindow::mapFromGlobal() can map any global coordinate to the correct
    //     local coordinate, as long as the coordinate is on screen and on the window.
    //
    // Open issue:
    //   - Mapping coordinates which are outside of the window is iffy; we might
    //     fall back to using/assuming the coordinate system for the main screen
    //     in this case.
    QPoint globalTopLeft = window.mapToGlobal(QPoint(0, 0));
    QSize foo = window.geometry().size() - QSize(1, 1);
    QPoint globalBottomRight = window.mapToGlobal(QPoint(foo.width(), foo.height()));

    QCOMPARE(app->screenAt(globalTopLeft), app->screens()[0]);
    QCOMPARE(app->screenAt(globalBottomRight), app->screens()[1]);
}

void tst_QHighDpi::mouseEvents_data()
{
    standardScreenDpiTestData();
}

void tst_QHighDpi::mouseEvents()
{
    QFETCH(QList<qreal>, dpiValues);
    std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(dpiValues));

    class MousePressTestWindow : public QWindow {
    public:
        QPoint m_mouseTestPoint;

        MousePressTestWindow(QWindow *parent = nullptr)
            :QWindow(parent)
        {

        }

        void mousePressEvent(QMouseEvent *ev) override
        {
            QCOMPARE(ev->position(), m_mouseTestPoint);
            if (devicePixelRatio() == 1 || devicePixelRatio() == 2) // ### off-by-one error on non-integer dpr
                QCOMPARE(mapFromGlobal(ev->globalPosition()), m_mouseTestPoint);
        }

    };

    // Verify mouse event coordinates for top-level and chlid windows on each screen
    for (QScreen *screen : app->screens()) {
        QPoint mouseTestPoint(10, 10);
        MousePressTestWindow topLevelWindow;
        topLevelWindow.m_mouseTestPoint = mouseTestPoint;
        topLevelWindow.resize(QSize(40, 40));
        topLevelWindow.setPosition(screen->geometry().center());
        topLevelWindow.show();

        QTest::mouseClick(&topLevelWindow, Qt::LeftButton, Qt::KeyboardModifiers(), mouseTestPoint);
        MousePressTestWindow childWindow(&topLevelWindow);
        childWindow.m_mouseTestPoint = mouseTestPoint;
        childWindow.resize(QSize(20, 20));
        childWindow.setPosition(QPoint(15, 15));
        childWindow.show();
        QTest::mouseClick(&childWindow, Qt::LeftButton, Qt::KeyboardModifiers(), mouseTestPoint);
    }

    // Verify mouse event coordinates for a window spanning screen 0 and screen 1
    QPoint screen0Center = app->screens()[0]->geometry().center();
    int screenWidth = app->screens()[0]->geometry().width();
    QSize windowSize = QSize(screenWidth - 10, 20);
    QRect windowGeometry = QRect(screen0Center, windowSize);
    windowGeometry.adjust(0, 0, -10, 0); // Make sure the center point is on screen 0
    MousePressTestWindow window;
    window.setGeometry(windowGeometry);
    window.show();

    QPoint screen0Point(QPoint(10,10));
    QPoint screen1Point(QPoint(windowSize.width() - 20,10));
    QCOMPARE(app->screenAt(window.mapToGlobal(screen0Point)), app->screens()[0]);
    QCOMPARE(app->screenAt(window.mapToGlobal(screen1Point)), app->screens()[1]);

    window.m_mouseTestPoint = screen0Point;
    QTest::mouseClick(&window, Qt::LeftButton, Qt::KeyboardModifiers(), screen0Point);
    window.m_mouseTestPoint = screen1Point;
    QTest::mouseClick(&window, Qt::LeftButton, Qt::KeyboardModifiers(), screen1Point);
}

void tst_QHighDpi::mouseVelocity_data()
{
    standardScreenDpiTestData();
}

void tst_QHighDpi::mouseVelocity()
{
    QFETCH(QList<qreal>, dpiValues);
    std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(dpiValues));

    class MouseVelocityTestWindow : public QWindow {
    public:
        QVector2D velocity;
        bool decel = false;

        bool event(QEvent *ev) override
        {
            if (!ev->isPointerEvent())
                qCDebug(lcTests) << ev;
            return QWindow::event(ev);
        }

        void mousePressEvent(QMouseEvent *ev) override
        {
            velocity = ev->points().first().velocity();
            qCDebug(lcTests) << "velocity" << velocity << ev;
        }

        void mouseMoveEvent(QMouseEvent *ev) override
        {
            velocity = ev->points().first().velocity();
            if (ev->buttons())
                qCDebug(lcTests) << "velocity" << velocity << ev;
        }
    };

    // Verify velocity direction and sign on each screen
    // FYI: Turn on the qt.pointer.velocity logging category to see how it's calculated
    for (QScreen *screen : app->screens()) {
        MouseVelocityTestWindow topLevelWindow;
        topLevelWindow.resize(QSize(120, 120));
        topLevelWindow.setPosition(screen->geometry().center());
        topLevelWindow.show();

        QPoint endP;
        qreal maxVx = 0;
        qreal maxVy = 0;
        qreal minVx = INT_MAX;
        qreal minVy = INT_MAX;
        for (int xDelta = 10; xDelta >= -10; xDelta -= 10) {
            for (int yDelta = 10; yDelta >= -10; yDelta -= 10) {
                QPoint p(60, 60);
                // move closer to p, decelerating, to get the velocity down to a small value
                for (int i = 0; i < 12; ++i) {
                    endP += (p - endP) / 4;
                    QTest::mouseMove(&topLevelWindow, endP, 3 * i);
                }
                qCDebug(lcTests) << "beginning drag with dx" << xDelta << "dy" << yDelta;
                QTest::mouseMove(&topLevelWindow, p, 10);
                QTest::mousePress(&topLevelWindow, Qt::LeftButton, {}, p);
                QVERIFY(qAbs(topLevelWindow.velocity.x()) < 50);
                QVERIFY(qAbs(topLevelWindow.velocity.y()) < 50);
                for (int i = 0; i < 4; ++i) {
                    p += QPoint(xDelta, yDelta);
                    QTest::mouseMove(&topLevelWindow, p, 10);
                    if (xDelta) {
                        // same sign and decent magnitude:
                        // 10 px in 10 ms =~ 1000 px / second; should be in logical coordinates on any screen
                        // but it's not exactly 1000 because of the Kalman filter
                        QVERIFY(topLevelWindow.velocity.x() * xDelta > 0);
                        QVERIFY(qAbs(topLevelWindow.velocity.x()) > 500);
                    } else {
                        QVERIFY(qAbs(topLevelWindow.velocity.x()) < 10);
                    }
                    if (yDelta) {
                        QVERIFY(topLevelWindow.velocity.y() * yDelta > 0);
                        QVERIFY(qAbs(topLevelWindow.velocity.y()) > 500);
                    } else {
                        QVERIFY(qAbs(topLevelWindow.velocity.y()) < 10);
                    }
                    maxVx = qMax(topLevelWindow.velocity.x(), maxVx);
                    maxVy = qMax(topLevelWindow.velocity.y(), maxVy);
                    minVx = qMin(topLevelWindow.velocity.x(), minVx);
                    minVy = qMin(topLevelWindow.velocity.y(), minVy);
                }
                QTest::mouseRelease(&topLevelWindow, Qt::LeftButton, {}, p);
                endP = p; // QED
            }
        }
        qCDebug(lcTests) << "mouse land speed record: forward" << maxVx << maxVy << "reverse" << minVx << minVy;
        // all drags were at the same speed, so max speed should be equal in each direction
        QVERIFY(qAbs(maxVx - maxVy) < 10);
        QVERIFY(qAbs(minVx - minVy) < 10);
        QVERIFY(maxVx + minVx < 10);
        QVERIFY(maxVy + minVy < 10);
    }
}

void tst_QHighDpi::setCursor_data()
{
    standardScreenDpiTestData();
}

void tst_QHighDpi::setCursor()
{
    QFETCH(QList<qreal>, dpiValues);
    std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(dpiValues));

    for (QScreen *screen : app->screens()) {
        QPoint center = screen->geometry().center();
        QCursor::setPos(center.x(), center.y());
        QCOMPARE(QCursor::pos(), center);
    }
}

void tst_QHighDpi::setGlobalFactorEmits()
{
    QList<qreal> dpiValues { 96, 96, 96 };
    std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(dpiValues));

    std::vector<std::unique_ptr<QSignalSpy>> spies;
    for (QScreen *screen : app->screens())
        spies.push_back(std::make_unique<QSignalSpy>(screen, &QScreen::geometryChanged));

    QHighDpiScaling::setGlobalFactor(2);

    for (const auto &spy : spies)
        QCOMPARE(spy->count(), 1);

    QHighDpiScaling::setGlobalFactor(1);
}

void tst_QHighDpi::setScreenFactorEmits()
{
    QList<qreal> dpiValues { 96, 96, 96 };
    std::unique_ptr<QGuiApplication> app(createStandardOffscreenApp(dpiValues));

    for (QScreen *screen : app->screens()) {
        QSignalSpy spy(screen, &QScreen::geometryChanged);
        QHighDpiScaling::setScreenFactor(screen, 2);
        QCOMPARE(spy.count(), 1);
    }
}

#include "tst_qhighdpi.moc"
QTEST_APPLESS_MAIN(tst_QHighDpi);
