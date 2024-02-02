// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>
#include <QBuffer>

#include <qpicture.h>
#include <qpainter.h>
#include <qimage.h>
#include <qpaintengine.h>
#include <qguiapplication.h>
#include <qscreen.h>
#include <limits.h>

#ifndef QT_NO_PICTURE

class tst_QPicture : public QObject
{
    Q_OBJECT

public:
    tst_QPicture();

private slots:
    void devType();
    void paintingActive();
    void boundingRect();
    void swap();
    void serialization();
    void save_restore();
    void boundaryValues_data();
    void boundaryValues();
};

tst_QPicture::tst_QPicture()
{
}

void tst_QPicture::devType()
{
    QPicture p;
    QCOMPARE( p.devType(), (int)QInternal::Picture );
}

void tst_QPicture::paintingActive()
{
    // actually implemented in QPainter but QPicture is a good
    // example of an external paint device
    QPicture p;
    QVERIFY( !p.paintingActive() );
    QPainter pt( &p );
    QVERIFY( p.paintingActive() );
    pt.end();
    QVERIFY( !p.paintingActive() );
}

void tst_QPicture::boundingRect()
{
    QPicture p1;
    // default value
    QVERIFY( !p1.boundingRect().isValid() );

    QRect r1( 20, 30, 5, 15 );
    p1.setBoundingRect( r1 );
    QCOMPARE( p1.boundingRect(), r1 );
    p1.setBoundingRect(QRect());

    QPainter pt( &p1 );
    pt.drawLine( 10, 20, 110, 80 );
    pt.end();

    // assignment and copy constructor
    QRect r2( 10, 20, 100, 60 );
    QCOMPARE( p1.boundingRect(), r2 );
    QPicture p2( p1 );
    QCOMPARE( p2.boundingRect(), r2 );
    QPicture p3;
    p3 = p1;
    QCOMPARE( p3.boundingRect(), r2 );

    {
        QPicture p4;
        QPainter p(&p4);
        p.drawLine(0, 0, 5, 0);
        p.drawLine(0, 0, 0, 5);
        p.end();

        QRect r3(0, 0, 5, 5);
        QCOMPARE(p4.boundingRect(), r3);
    }
}

void tst_QPicture::swap()
{
    QPicture p1, p2;
    QPainter(&p1).drawLine(0, 0, 5, 5);
    QPainter(&p2).drawLine(0, 3, 3, 0);
    QCOMPARE(p1.boundingRect(), QRect(0,0,5,5));
    QCOMPARE(p2.boundingRect(), QRect(0,0,3,3));
    p1.swap(p2);
    QCOMPARE(p1.boundingRect(), QRect(0,0,3,3));
    QCOMPARE(p2.boundingRect(), QRect(0,0,5,5));
}

Q_DECLARE_METATYPE(QDataStream::Version)
Q_DECLARE_METATYPE(QPicture)

void ensureSerializesCorrectly(const QPicture &picture, QDataStream::Version version)
 {
    QDataStream stream;

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    stream.setDevice(&buffer);
    stream.setVersion(version);
    stream << picture;
    buffer.close();

    buffer.open(QIODevice::ReadOnly);
    QPicture readpicture;
    stream >> readpicture;
    QVERIFY2(memcmp(picture.data(), readpicture.data(), picture.size()) == 0,
        qPrintable(QString::fromLatin1("Picture data does not compare equal for QDataStream version %1").arg(version)));
}

class PaintEngine : public QPaintEngine
{
public:
    PaintEngine() : QPaintEngine() {}
    bool begin(QPaintDevice *) override { return true; }
    bool end() override { return true; }
    void updateState(const QPaintEngineState &) override {}
    void drawPixmap(const QRectF &, const QPixmap &, const QRectF &) override {}
    Type type() const override { return Raster; }

    QFont font() { return state->font(); }
};

class Picture : public QPicture
{
public:
    Picture() : QPicture() {}
    QPaintEngine *paintEngine() const override { return (QPaintEngine*)&mPaintEngine; }
private:
    PaintEngine mPaintEngine;
};

void tst_QPicture::serialization()
{
    QDataStream stream;
    const int thisVersion = stream.version();

    for (int version = QDataStream::Qt_1_0; version <= thisVersion; ++version) {
        const QDataStream::Version versionEnum = static_cast<QDataStream::Version>(version);

        {
            // streaming of null pictures
            ensureSerializesCorrectly(QPicture(), versionEnum);
        }
        {
            // picture with a simple line, checking bitwise equality
            QPicture picture;
            QPainter painter(&picture);
            painter.drawLine(10, 20, 30, 40);
            ensureSerializesCorrectly(picture, versionEnum);
        }
    }

    {
        // Test features that were added after Qt 4.5, as that was hard-coded as the major
        // version for a while, which was incorrect. In this case, we'll test font hints.
        QPicture picture;
        QPainter painter;
        QFont font;
        font.setStyleName("Blah");
        font.setHintingPreference(QFont::PreferFullHinting);
        painter.begin(&picture);
        painter.setFont(font);
        painter.drawText(20, 20, "Hello");
        painter.end();

        Picture customPicture;
        painter.begin(&customPicture);
        picture.play(&painter);
        const QFont actualFont = ((PaintEngine*)customPicture.paintEngine())->font();
        painter.end();
        QCOMPARE(actualFont.styleName(), QStringLiteral("Blah"));
        QCOMPARE(actualFont.hintingPreference(), QFont::PreferFullHinting);
    }
}

static QRectF scaleRect(const QRectF &rect, qreal xf, qreal yf)
{
    return QRectF(rect.left() * xf, rect.top() * yf, rect.width() * xf, rect.height() * yf);
}

static void paintStuff(QPainter *p)
{
    const QScreen *screen = QGuiApplication::primaryScreen();
    // Calculate factors from the screen resolution against QPicture's 96DPI
    // (enforced by Qt::AA_Use96Dpi as set by QTEST_MAIN).
    const qreal xf = qreal(p->device()->logicalDpiX()) / screen->logicalDotsPerInchX();
    const qreal yf = qreal(p->device()->logicalDpiY()) / screen->logicalDotsPerInchY();
    p->drawRect(scaleRect(QRectF(100, 100, 100, 100), xf, yf));
    p->save();
    p->translate(10 * xf, 10 * yf);
    p->restore();
    p->drawRect(scaleRect(QRectF(100, 100, 100, 100), xf, yf));
}

/* See task: 41469
   Problem is that the state is not properly restored if the basestate of
   the painter is different when the picture data was created compared to
   the base state of the painter when it is played back.
 */
void tst_QPicture::save_restore()
{
    QPicture pic;
    QPainter p;
    p.begin(&pic);
    paintStuff(&p);
    p.end();

    QPixmap pix1(300, 300);
    pix1.fill(Qt::white);
    p.begin(&pix1);
    p.drawPicture(50, 50, pic);
    p.end();

    QPixmap pix2(300, 300);
    pix2.fill(Qt::white);
    p.begin(&pix2);
    p.translate(50, 50);
    paintStuff(&p);
    p.end();

    QVERIFY( pix1.toImage() == pix2.toImage() );
}

void tst_QPicture::boundaryValues_data()
{
    QTest::addColumn<int>("x");
    QTest::addColumn<int>("y");
    QTest::newRow("max x") << INT_MAX << 50;
    QTest::newRow("max y") << 50 << INT_MAX;
    QTest::newRow("max x and y") << INT_MAX << INT_MAX;

    QTest::newRow("min x") << INT_MIN << 50;
    QTest::newRow("min y") << 50 << INT_MIN;
    QTest::newRow("min x and y") << INT_MIN << INT_MIN;

    QTest::newRow("min x, max y") << INT_MIN << INT_MAX;
    QTest::newRow("max x, min y") << INT_MAX << INT_MIN;
}

void tst_QPicture::boundaryValues()
{
    QPicture picture;

    QPainter painter;
    painter.begin(&picture);

    QFETCH(int, x);
    QFETCH(int, y);
    painter.drawPoint(QPoint(x, y));

    painter.end();
}

QTEST_MAIN(tst_QPicture)
#include "tst_qpicture.moc"

#endif // QT_NO_PICTURE
