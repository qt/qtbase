// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qfontdatabase.h>
#include <qpaintengine.h>
#include <qpainter.h>
#include <qpixmap.h>

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformfontdatabase.h>
#include <QtGui/qpa/qplatformintegration.h>

class tst_QPaintEngine : public QObject
{
Q_OBJECT

public:
    tst_QPaintEngine();
    virtual ~tst_QPaintEngine();

private slots:
    void getSetCheck();
    void colorFontGlyphImages();
};

tst_QPaintEngine::tst_QPaintEngine()
{
}

tst_QPaintEngine::~tst_QPaintEngine()
{
}

class MyPaintEngine : public QPaintEngine
{
public:
    MyPaintEngine() : QPaintEngine() {}
    bool begin(QPaintDevice *) override { return true; }
    bool end() override { return true; }
    void updateState(const QPaintEngineState &) override {}
    void drawPixmap(const QRectF &, const QPixmap &, const QRectF &) override {}
    Type type() const override { return Raster; }
};

// Testing get/set functions
void tst_QPaintEngine::getSetCheck()
{
    MyPaintEngine obj1;
    // QPaintDevice * QPaintEngine::paintDevice()
    // void QPaintEngine::setPaintDevice(QPaintDevice *)
    QPixmap *var1 = new QPixmap;
    obj1.setPaintDevice(var1);
    QCOMPARE((QPaintDevice *)var1, obj1.paintDevice());
    obj1.setPaintDevice((QPaintDevice *)0);
    QCOMPARE((QPaintDevice *)0, obj1.paintDevice());
    delete var1;
}

class RecordingPaintEngine : public QPaintEngine
{
public:
    struct DrawnImage
    {
        QPointF position;
        QImage image;
    };

    RecordingPaintEngine() : QPaintEngine(AllFeatures) {}
    bool begin(QPaintDevice *) override { return true; }
    bool end() override { return true; }
    void updateState(const QPaintEngineState &) override {}
    void drawPixmap(const QRectF &, const QPixmap &, const QRectF &) override {}
    void drawImage(const QRectF &r, const QImage &image, const QRectF &,
                   Qt::ImageConversionFlags) override
    {
        drawnImages.append({r.topLeft(), image});
    }
    Type type() const override { return User; }

    QList<DrawnImage> drawnImages;
};

class RecordingPaintDevice : public QPaintDevice
{
public:
    QPaintEngine *paintEngine() const override { return &m_engine; }

    int metric(PaintDeviceMetric m) const override
    {
        switch (m) {
        case PdmWidth:
        case PdmHeight:
            return 1000;
        case PdmWidthMM:
        case PdmHeightMM:
            return 353;
        case PdmNumColors:
            return 0;
        case PdmDepth:
            return 32;
        case PdmDpiX:
        case PdmDpiY:
        case PdmPhysicalDpiX:
        case PdmPhysicalDpiY:
            return 72;
        default:
            return QPaintDevice::metric(m);
        }
    }

    mutable RecordingPaintEngine m_engine;
};

static bool imageContainsColor(const QImage &img, const QColor &color)
{
    const QImage argb = img.convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < argb.height(); ++y) {
        for (int x = 0; x < argb.width(); ++x) {
            const QColor pixel = argb.pixelColor(x, y);
            if (pixel.alpha() > 128 && qAbs(pixel.red() - color.red()) < 64
                && qAbs(pixel.green() - color.green()) < 64
                && qAbs(pixel.blue() - color.blue()) < 64) {
                return true;
            }
        }
    }
    return false;
}

// The generic drawTextItem() fallback draws color-font glyphs as images.
// The images must sit on the baseline according to each glyph's own
// bearings and must be rendered in the pen color.
void tst_QPaintEngine::colorFontGlyphImages()
{
    {
        QPlatformFontDatabase *pfdb =
            QGuiApplicationPrivate::platformIntegration()->fontDatabase();
        if (!pfdb->supportsColrv0Fonts())
            QSKIP("This test depends on COLRv0 support.");
    }

    const QString fontFile = QFINDTESTDATA("QtEmojiTestFont-Regular.ttf");
    QVERIFY(!fontFile.isEmpty());

    int id = -1;
    auto cleanup = qScopeGuard([&id] {
        if (id >= 0)
            QFontDatabase::removeApplicationFont(id);
    });

    id = QFontDatabase::addApplicationFont(fontFile);
    QVERIFY(id >= 0);

    const QStringList families = QFontDatabase::applicationFontFamilies(id);
    QVERIFY(!families.isEmpty());

    RecordingPaintDevice device;
    QPainter painter(&device);
    QFont font(families.first());
    font.setPixelSize(64);
    painter.setFont(font);
    painter.setPen(Qt::red);
    // '.' is small, '0' is tall; both sit exactly on the baseline
    painter.drawText(QPointF(10, 100), QStringLiteral(".0"));
    painter.end();

    const auto &images = device.m_engine.drawnImages;
    QCOMPARE(images.size(), 2);

    const auto &x = images.at(0);
    const auto &l = images.at(1);

    // 'l' must be noticeably taller than 'x' for the baseline check to
    // mean anything
    QCOMPARE_GT(l.image.height(), x.image.height() + 5);

    // Both glyph images must end at the baseline. Without the per-glyph
    // bearing offsets they are aligned at their top edges instead, so their
    // bottom edges differ by the height difference.
    const qreal xBottom = x.position.y() + x.image.height();
    const qreal lBottom = l.position.y() + l.image.height();
    QCOMPARE_LE(qAbs(xBottom - lBottom), 2.0);

    // Monochrome glyphs of a color font must be rendered in the pen color,
    // not in black
    QVERIFY(imageContainsColor(x.image, Qt::red));
    QVERIFY(imageContainsColor(l.image, Qt::red));
}

QTEST_MAIN(tst_QPaintEngine)
#include "tst_qpaintengine.moc"
