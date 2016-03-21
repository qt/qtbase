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
#include <QtGui/QGuiApplication>
#include <QtGui/QPainter>
#include <QtGui/QImage>

#include <qstatictext.h>
#include <qpaintengine.h>

#ifdef QT_BUILD_INTERNAL
#include <private/qstatictext_p.h>
#endif

// #define DEBUG_SAVE_IMAGE

static inline QImage blankSquare()
{
    // a "blank" square; we compare against in our testfunctions to verify
    // that we have actually painted something
    QPixmap pm(1000, 1000);
    pm.fill(Qt::white);
    return pm.toImage();
}

class tst_QStaticText: public QObject
{
    Q_OBJECT
public:
    tst_QStaticText() : m_whiteSquare(blankSquare()) {}

private slots:
    void constructionAndDestruction();
    void drawToPoint_data();
    void drawToPoint();
    void drawToRect_data();
    void drawToRect();
    void setFont();
    void setTextWidth();
    void prepareToCorrectData();
    void prepareToWrongData();

    void copyConstructor();

    void translatedPainter();
    void rotatedPainter();
    void scaledPainter();
    void projectedPainter();
#if 0
    void rotatedScaledAndTranslatedPainter_data();
    void rotatedScaledAndTranslatedPainter();
#endif
    void transformationChanged();

    void plainTextVsRichText();

    void setPenPlainText_data();
    void setPenPlainText();
    void setPenRichText();
    void richTextOverridesPen();

    void drawStruckOutText();
    void drawOverlinedText();
    void drawUnderlinedText();

    void unprintableCharacter_qtbug12614();

#ifdef QT_BUILD_INTERNAL
    void underlinedColor_qtbug20159();
    void textDocumentColor();
#endif

private:
    bool supportsTransformations() const;

    const QImage m_whiteSquare;
};

Q_DECLARE_METATYPE(QImage::Format);

void tst_QStaticText::constructionAndDestruction()
{
    QStaticText text("My text");
}

void tst_QStaticText::copyConstructor()
{
    QStaticText text(QLatin1String("My text"));

    QTextOption textOption(Qt::AlignRight);
    text.setTextOption(textOption);

    text.setPerformanceHint(QStaticText::AggressiveCaching);
    text.setTextWidth(123.456);
    text.setTextFormat(Qt::PlainText);

    QStaticText copiedText(text);
    copiedText.setText(QLatin1String("Other text"));

    QCOMPARE(copiedText.textOption().alignment(), Qt::AlignRight);
    QCOMPARE(copiedText.performanceHint(), QStaticText::AggressiveCaching);
    QCOMPARE(copiedText.textWidth(), 123.456);
    QCOMPARE(copiedText.textFormat(), Qt::PlainText);

    QStaticText otherCopiedText(copiedText);
    otherCopiedText.setTextWidth(789);

    QCOMPARE(otherCopiedText.text(), QString::fromLatin1("Other text"));
}

Q_DECLARE_METATYPE(QStaticText::PerformanceHint)
void tst_QStaticText::drawToPoint_data()
{
    QTest::addColumn<QStaticText::PerformanceHint>("performanceHint");

    QTest::newRow("Moderate caching") << QStaticText::ModerateCaching;
    QTest::newRow("Aggressive caching") << QStaticText::AggressiveCaching;
}

void tst_QStaticText::drawToPoint()
{
    QFETCH(QStaticText::PerformanceHint, performanceHint);

    QPixmap imageDrawText(1000, 1000);
    imageDrawText.fill(Qt::white);
    {
        QPainter p(&imageDrawText);
        p.drawText(11, 12, "Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
    }

    QPixmap imageDrawStaticText(1000, 1000);
    imageDrawStaticText.fill(Qt::white);
    {
        QPainter p(&imageDrawStaticText);
        QStaticText text("Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
        text.setTextFormat(Qt::PlainText);
        text.setPerformanceHint(performanceHint);
        p.drawStaticText(QPointF(11, 12 - QFontMetricsF(p.font()).ascent()), text);
    }

    QVERIFY(imageDrawText.toImage() != m_whiteSquare);
    QCOMPARE(imageDrawStaticText, imageDrawText);
}

void tst_QStaticText::drawToRect_data()
{
    QTest::addColumn<QStaticText::PerformanceHint>("performanceHint");

    QTest::newRow("Moderate caching") << QStaticText::ModerateCaching;
    QTest::newRow("Aggressive caching") << QStaticText::AggressiveCaching;
}

void tst_QStaticText::drawToRect()
{
    QFETCH(QStaticText::PerformanceHint, performanceHint);

    QPixmap imageDrawText(1000, 1000);
    imageDrawText.fill(Qt::white);
    {
        QPainter p(&imageDrawText);
        p.drawText(QRectF(11, 12, 10, 500), "Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
    }

    QPixmap imageDrawStaticText(1000, 1000);
    imageDrawStaticText.fill(Qt::white);
    {
        QPainter p(&imageDrawStaticText);
        QStaticText text("Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
        text.setTextWidth(10),
        p.setClipRect(QRectF(11, 12, 10, 500));
        text.setPerformanceHint(performanceHint);
        text.setTextFormat(Qt::PlainText);
        p.drawStaticText(QPointF(11, 12), text);
    }

#if defined(DEBUG_SAVE_IMAGE)
    imageDrawText.save("drawToRect_imageDrawText.png");
    imageDrawStaticText.save("drawToRect_imageDrawStaticText.png");
#endif

    QVERIFY(imageDrawText.toImage() != m_whiteSquare);
    QCOMPARE(imageDrawStaticText, imageDrawText);
}

void tst_QStaticText::prepareToCorrectData()
{
    QTransform transform;
    transform.scale(2.0, 2.0);
    transform.translate(100, 10);
    transform.rotate(90, Qt::ZAxis);

    QPixmap imageDrawText(1000, 1000);
    imageDrawText.fill(Qt::white);
    {
        QPainter p(&imageDrawText);
        p.setTransform(transform);
        p.drawText(11, 12, "Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
    }

    QPixmap imageDrawStaticText(1000, 1000);
    imageDrawStaticText.fill(Qt::white);
    {
        QPainter p(&imageDrawStaticText);
        p.setTransform(transform);
        QStaticText text("Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
        text.prepare(transform, p.font());
        text.setTextFormat(Qt::PlainText);
        p.drawStaticText(QPointF(11, 12  - QFontMetricsF(p.font()).ascent()), text);
    }

#if defined(DEBUG_SAVE_IMAGE)
    imageDrawText.save("prepareToCorrectData_imageDrawText.png");
    imageDrawStaticText.save("prepareToCorrectData_imageDrawStaticText.png");
#endif

    QVERIFY(imageDrawText.toImage() != m_whiteSquare);

    if (!supportsTransformations())
      QEXPECT_FAIL("", "Graphics system does not support transformed text on this platform", Abort);
    QCOMPARE(imageDrawStaticText, imageDrawText);
}

void tst_QStaticText::prepareToWrongData()
{
    QTransform transform;
    transform.scale(2.0, 2.0);
    transform.rotate(90, Qt::ZAxis);

    QPixmap imageDrawText(1000, 1000);
    imageDrawText.fill(Qt::white);
    {
        QPainter p(&imageDrawText);
        p.drawText(11, 12, "Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
    }

    QPixmap imageDrawStaticText(1000, 1000);
    imageDrawStaticText.fill(Qt::white);
    {
        QPainter p(&imageDrawStaticText);
        QStaticText text("Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
        text.prepare(transform, p.font());
        text.setTextFormat(Qt::PlainText);
        p.drawStaticText(QPointF(11, 12  - QFontMetricsF(p.font()).ascent()), text);
    }

    QVERIFY(imageDrawText.toImage() != m_whiteSquare);
    QCOMPARE(imageDrawStaticText, imageDrawText);
}


void tst_QStaticText::setFont()
{
    QFont font = QGuiApplication::font();
    font.setBold(true);
    font.setPointSize(28);

    QPixmap imageDrawText(1000, 1000);
    imageDrawText.fill(Qt::white);
    {
        QPainter p(&imageDrawText);
        p.drawText(QRectF(0, 0, 1000, 1000), 0, "Lorem ipsum dolor sit amet, consectetur adipiscing elit.");

        p.setFont(font);
        p.drawText(QRectF(11, 120, 1000, 1000), 0, "Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
    }

    QPixmap imageDrawStaticText(1000, 1000);
    imageDrawStaticText.fill(Qt::white);
    {
        QPainter p(&imageDrawStaticText);

        QStaticText text;
        text.setText("Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
        text.setTextFormat(Qt::PlainText);

        p.drawStaticText(0, 0, text);

        p.setFont(font);
        p.drawStaticText(11, 120, text);
    }

#if defined(DEBUG_SAVE_IMAGE)
    imageDrawText.save("setFont_imageDrawText.png");
    imageDrawStaticText.save("setFont_imageDrawStaticText.png");
#endif

    QVERIFY(imageDrawText.toImage() != m_whiteSquare);
    QCOMPARE(imageDrawStaticText, imageDrawText);
}

void tst_QStaticText::setTextWidth()
{
    QPixmap imageDrawText(1000, 1000);
    imageDrawText.fill(Qt::white);
    {
        QPainter p(&imageDrawText);
        p.drawText(QRectF(11, 12, 10, 500), "Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
    }

    QPixmap imageDrawStaticText(1000, 1000);
    imageDrawStaticText.fill(Qt::white);
    {
        QPainter p(&imageDrawStaticText);
        QStaticText text("Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
        text.setTextWidth(10);
        p.setClipRect(QRectF(11, 12, 10, 500));
        p.drawStaticText(QPointF(11, 12), text);
    }

    QVERIFY(imageDrawText.toImage() != m_whiteSquare);
    QCOMPARE(imageDrawStaticText, imageDrawText);
}

void tst_QStaticText::translatedPainter()
{
    QPixmap imageDrawText(1000, 1000);
    imageDrawText.fill(Qt::white);
    {
        QPainter p(&imageDrawText);
        p.translate(100, 200);

        p.drawText(11, 12, "Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
    }

    QPixmap imageDrawStaticText(1000, 1000);
    imageDrawStaticText.fill(Qt::white);
    {
        QPainter p(&imageDrawStaticText);
        p.translate(100, 200);

        QStaticText text("Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
        text.setTextFormat(Qt::PlainText);

        p.drawStaticText(QPointF(11, 12 - QFontMetricsF(p.font()).ascent()), text);
    }

    QVERIFY(imageDrawText.toImage() != m_whiteSquare);
    QCOMPARE(imageDrawStaticText, imageDrawText);
}

bool tst_QStaticText::supportsTransformations() const
{
    QPixmap pm(10, 10);
    QPainter p(&pm);
    return p.paintEngine()->type() != QPaintEngine::OpenGL;
}

void tst_QStaticText::rotatedPainter()
{
    QPixmap imageDrawText(1000, 1000);
    imageDrawText.fill(Qt::white);
    {
        QPainter p(&imageDrawText);
        p.rotate(30.0);
        p.drawText(QRectF(0, 0, 1000, 100), 0, "Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
    }

    QPixmap imageDrawStaticText(1000, 1000);
    imageDrawStaticText.fill(Qt::white);
    {
        QStaticText text("Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
        text.setTextFormat(Qt::PlainText);

        QPainter p(&imageDrawStaticText);
        p.rotate(30.0);
        p.drawStaticText(QPoint(0, 0), text);
    }

#if defined(DEBUG_SAVE_IMAGE)
    imageDrawText.save("rotatedPainter_imageDrawText.png");
    imageDrawStaticText.save("rotatedPainter_imageDrawStaticText.png");
#endif

    QVERIFY(imageDrawText.toImage() != m_whiteSquare);

    if (!supportsTransformations())
      QEXPECT_FAIL("", "Graphics system does not support transformed text on this platform", Abort);
    QCOMPARE(imageDrawStaticText, imageDrawText);
}

void tst_QStaticText::scaledPainter()
{
    QPixmap imageDrawText(1000, 1000);
    imageDrawText.fill(Qt::white);
    {
        QPainter p(&imageDrawText);
        p.scale(2.0, 0.2);

        p.drawText(11, 12, "Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
    }

    QPixmap imageDrawStaticText(1000, 1000);
    imageDrawStaticText.fill(Qt::white);
    {
        QPainter p(&imageDrawStaticText);
        p.scale(2.0, 0.2);

        QStaticText text("Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
        text.setTextFormat(Qt::PlainText);

        p.drawStaticText(QPointF(11, 12 - QFontMetricsF(p.font()).ascent()), text);
    }

    QVERIFY(imageDrawText.toImage() != m_whiteSquare);

    if (!supportsTransformations())
      QEXPECT_FAIL("", "Graphics system does not support transformed text on this platform", Abort);
    QCOMPARE(imageDrawStaticText, imageDrawText);
}

void tst_QStaticText::projectedPainter()
{
    QTransform transform;
    transform.rotate(90, Qt::XAxis);

    QPixmap imageDrawText(1000, 1000);
    imageDrawText.fill(Qt::white);
    {
        QPainter p(&imageDrawText);
        p.setTransform(transform);

        p.drawText(11, 12, "Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
    }

    QPixmap imageDrawStaticText(1000, 1000);
    imageDrawStaticText.fill(Qt::white);
    {
        QPainter p(&imageDrawStaticText);
        p.setTransform(transform);

        QStaticText text("Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
        text.setTextFormat(Qt::PlainText);

        p.drawStaticText(QPointF(11, 12 - QFontMetricsF(p.font()).ascent()), text);
    }

    QCOMPARE(imageDrawStaticText, imageDrawText);
}

#if 0
void tst_QStaticText::rotatedScaledAndTranslatedPainter_data()
{
    QTest::addColumn<qreal>("offset");

    for (int i=0; i<100; ++i) {
        qreal offset = 300 + i / 100.;
        QTest::newRow(QByteArray::number(offset).constData()) << offset;
    }
}

void tst_QStaticText::rotatedScaledAndTranslatedPainter()
{
    QFETCH(qreal, offset);

    QPixmap imageDrawText(1000, 1000);
    imageDrawText.fill(Qt::white);
    {
        QPainter p(&imageDrawText);
        p.translate(offset, 0);
        p.rotate(45.0);
        p.scale(2.0, 2.0);
        p.translate(100, 200);

        p.drawText(11, 12, "Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
    }

    QPixmap imageDrawStaticText(1000, 1000);
    imageDrawStaticText.fill(Qt::white);
    {
        QPainter p(&imageDrawStaticText);
        p.translate(offset, 0);
        p.rotate(45.0);
        p.scale(2.0, 2.0);
        p.translate(100, 200);

        QStaticText text("Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
        text.setTextFormat(Qt::PlainText);

        p.drawStaticText(QPointF(11, 12 - QFontMetricsF(p.font()).ascent()), text);
    }

#if defined(DEBUG_SAVE_IMAGE)
    imageDrawText.save("rotatedScaledAndPainter_imageDrawText.png");
    imageDrawStaticText.save("rotatedScaledAndPainter_imageDrawStaticText.png");
#endif

    QVERIFY(imageDrawText.toImage() != m_whiteSquare);

    if (!supportsTransformations())
      QEXPECT_FAIL("", "Graphics system does not support transformed text on this platform", Abort);
    QCOMPARE(imageDrawStaticText, imageDrawText);
}
#endif

void tst_QStaticText::transformationChanged()
{
    QPixmap imageDrawText(1000, 1000);
    imageDrawText.fill(Qt::white);
    {
        QPainter p(&imageDrawText);
        p.rotate(33.0);
        p.scale(0.5, 0.7);

        p.drawText(QRectF(0, 0, 1000, 1000), 0, "Lorem ipsum dolor sit amet, consectetur adipiscing elit.");

        p.scale(2.0, 2.5);
        p.drawText(QRectF(0, 0, 1000, 1000), 0, "Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
    }

    QPixmap imageDrawStaticText(1000, 1000);
    imageDrawStaticText.fill(Qt::white);
    {
        QPainter p(&imageDrawStaticText);
        p.rotate(33.0);
        p.scale(0.5, 0.7);

        QStaticText text("Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
        text.setTextFormat(Qt::PlainText);

        p.drawStaticText(QPointF(0, 0), text);

        p.scale(2.0, 2.5);
        p.drawStaticText(QPointF(0, 0), text);
    }

#if defined(DEBUG_SAVE_IMAGE)
    imageDrawText.save("transformationChanged_imageDrawText.png");
    imageDrawStaticText.save("transformationChanged_imageDrawStaticText.png");
#endif

    QVERIFY(imageDrawText.toImage() != m_whiteSquare);

    if (!supportsTransformations())
      QEXPECT_FAIL("", "Graphics system does not support transformed text on this platform", Abort);
    QCOMPARE(imageDrawStaticText, imageDrawText);
}

void tst_QStaticText::plainTextVsRichText()
{
    QPixmap imagePlainText(1000, 1000);
    imagePlainText.fill(Qt::white);
    {
        QPainter p(&imagePlainText);

        QStaticText staticText;
        staticText.setText("FOObar");
        staticText.setTextFormat(Qt::PlainText);

        p.drawStaticText(10, 10, staticText);
    }

    QPixmap imageRichText(1000, 1000);
    imageRichText.fill(Qt::white);
    {
        QPainter p(&imageRichText);

        QStaticText staticText;
        staticText.setText("<html><body>FOObar</body></html>");
        staticText.setTextFormat(Qt::RichText);

        p.drawStaticText(10, 10, staticText);
    }

#if defined(DEBUG_SAVE_IMAGE)
    imagePlainText.save("plainTextVsRichText_imagePlainText.png");
    imageRichText.save("plainTextVsRichText_imageRichText.png");
#endif

    QVERIFY(imagePlainText.toImage() != m_whiteSquare);
    QCOMPARE(imagePlainText, imageRichText);
}

static bool checkPixels(const QImage &image,
                        Qt::GlobalColor expectedColor1, Qt::GlobalColor expectedColor2,
                        QByteArray *errorMessage)
{
    const QRgb expectedRgb1 = QColor(expectedColor1).rgba();
    const QRgb expectedRgb2 = QColor(expectedColor2).rgba();

    for (int x = 0, w = image.width(); x < w; ++x) {
        for (int y = 0, h = image.height(); y < h; ++y) {
            const QRgb pixel = image.pixel(x, y);
            if (pixel != expectedRgb1 && pixel != expectedRgb2) {
                QString message;
                QDebug(&message) << "Color mismatch in image" << image
                    << "at" << x << ',' << y << ':' << showbase << hex << pixel
                    << "(expected: " << expectedRgb1 << ',' << expectedRgb2 << ')';
                *errorMessage = message.toLocal8Bit();
                return false;
            }
        }
    }
    return true;
}

void tst_QStaticText::setPenPlainText_data()
{
    QTest::addColumn<QImage::Format>("format");

    QTest::newRow("argb32pm") << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("rgb32") << QImage::Format_RGB32;
    QTest::newRow("rgba8888pm") << QImage::Format_RGBA8888_Premultiplied;
    QTest::newRow("rgbx8888") << QImage::Format_RGBX8888;
}

void tst_QStaticText::setPenPlainText()
{
    QFETCH(QImage::Format, format);

    QFont font = QGuiApplication::font();
    font.setStyleStrategy(QFont::NoAntialias);

    QFontMetricsF fm(font);
    QImage image(qCeil(fm.width("XXXXX")), qCeil(fm.height()), format);
    image.fill(Qt::white);
    {
        QPainter p(&image);
        p.setFont(font);
        p.setPen(Qt::yellow);

        QStaticText staticText("XXXXX");
        staticText.setTextFormat(Qt::PlainText);
        p.drawStaticText(0, 0, staticText);
    }

    QByteArray errorMessage;
    QVERIFY2(checkPixels(image, Qt::yellow, Qt::white, &errorMessage),
             errorMessage.constData());
}

void tst_QStaticText::setPenRichText()
{
    QFont font = QGuiApplication::font();
    font.setStyleStrategy(QFont::NoAntialias);

    QFontMetricsF fm(font);
    QPixmap image(qCeil(fm.width("XXXXX")), qCeil(fm.height()));
    image.fill(Qt::white);
    {
        QPainter p(&image);
        p.setFont(font);
        p.setPen(Qt::green);

        QStaticText staticText;
        staticText.setText("<html><body>XXXXX</body></html>");
        staticText.setTextFormat(Qt::RichText);
        p.drawStaticText(0, 0, staticText);
    }

    QByteArray errorMessage;
    QVERIFY2(checkPixels(image.toImage(), Qt::green, Qt::white, &errorMessage),
             errorMessage.constData());
}

void tst_QStaticText::richTextOverridesPen()
{
    QFont font = QGuiApplication::font();
    font.setStyleStrategy(QFont::NoAntialias);

    QFontMetricsF fm(font);
    QPixmap image(qCeil(fm.width("XXXXX")), qCeil(fm.height()));
    image.fill(Qt::white);
    {
        QPainter p(&image);
        p.setFont(font);
        p.setPen(Qt::green);

        QStaticText staticText;
        staticText.setText("<html><body><font color=\"#ff0000\">XXXXX</font></body></html>");
        staticText.setTextFormat(Qt::RichText);
        p.drawStaticText(0, 0, staticText);
    }

    QByteArray errorMessage;
    QVERIFY2(checkPixels(image.toImage(), Qt::red, Qt::white, &errorMessage),
             errorMessage.constData());
}

void tst_QStaticText::drawStruckOutText()
{
    QPixmap imageDrawText(1000, 1000);
    QPixmap imageDrawStaticText(1000, 1000);

    imageDrawText.fill(Qt::white);
    imageDrawStaticText.fill(Qt::white);

    QString s = QString::fromLatin1("Foobar");

    QFont font;
    font.setStrikeOut(true);

    {
        QPainter p(&imageDrawText);
        p.setFont(font);
        p.drawText(QPointF(50, 50), s);
    }

    {
        QPainter p(&imageDrawStaticText);
        QStaticText text = QStaticText(s);
        p.setFont(font);
        p.drawStaticText(QPointF(50, 50 - QFontMetricsF(p.font()).ascent()), text);
    }

#if defined(DEBUG_SAVE_IMAGE)
    imageDrawText.save("drawStruckOutText_imageDrawText.png");
    imageDrawStaticText.save("drawStruckOutText_imageDrawStaticText.png");
#endif

    QVERIFY(imageDrawText.toImage() != m_whiteSquare);
    QCOMPARE(imageDrawText, imageDrawStaticText);
}

void tst_QStaticText::drawOverlinedText()
{
    QPixmap imageDrawText(1000, 1000);
    QPixmap imageDrawStaticText(1000, 1000);

    imageDrawText.fill(Qt::white);
    imageDrawStaticText.fill(Qt::white);

    QString s = QString::fromLatin1("Foobar");

    QFont font;
    font.setOverline(true);

    {
        QPainter p(&imageDrawText);
        p.setFont(font);
        p.drawText(QPointF(50, 50), s);
    }

    {
        QPainter p(&imageDrawStaticText);
        QStaticText text = QStaticText(s);
        p.setFont(font);
        p.drawStaticText(QPointF(50, 50 - QFontMetricsF(p.font()).ascent()), text);
    }

#if defined(DEBUG_SAVE_IMAGE)
    imageDrawText.save("drawOverlinedText_imageDrawText.png");
    imageDrawStaticText.save("drawOverlinedText_imageDrawStaticText.png");
#endif

    QVERIFY(imageDrawText.toImage() != m_whiteSquare);
    QCOMPARE(imageDrawText, imageDrawStaticText);
}

void tst_QStaticText::drawUnderlinedText()
{
    QPixmap imageDrawText(1000, 1000);
    QPixmap imageDrawStaticText(1000, 1000);

    imageDrawText.fill(Qt::white);
    imageDrawStaticText.fill(Qt::white);

    QString s = QString::fromLatin1("Foobar");

    QFont font;
    font.setUnderline(true);

    {
        QPainter p(&imageDrawText);
        p.setFont(font);
        p.drawText(QPointF(50, 50), s);
    }

    {
        QPainter p(&imageDrawStaticText);
        QStaticText text = QStaticText(s);
        p.setFont(font);
        p.drawStaticText(QPointF(50, 50 - QFontMetricsF(p.font()).ascent()), text);
    }

#if defined(DEBUG_SAVE_IMAGE)
    imageDrawText.save("drawUnderlinedText_imageDrawText.png");
    imageDrawStaticText.save("drawUnderlinedText_imageDrawStaticText.png");
#endif

    QCOMPARE(imageDrawText, imageDrawStaticText);
}

void tst_QStaticText::unprintableCharacter_qtbug12614()
{
    QString s(QChar(0x200B)); // U+200B, ZERO WIDTH SPACE

    QStaticText staticText(s);

    QVERIFY(staticText.size().isValid()); // Force layout. Should not crash.
}

#ifdef QT_BUILD_INTERNAL
void tst_QStaticText::underlinedColor_qtbug20159()
{
    QString multiScriptText;
    multiScriptText += QChar(0x0410); // Cyrillic 'A'
    multiScriptText += QLatin1Char('A');

    QStaticText staticText(multiScriptText);

    QFont font;
    font.setUnderline(true);

    staticText.prepare(QTransform(), font);

    QStaticTextPrivate *d = QStaticTextPrivate::get(&staticText);
    QCOMPARE(d->itemCount, 2);

    // The pen should not be marked as dirty when drawing the underline
    QVERIFY(!d->items[0].color.isValid());
    QVERIFY(!d->items[1].color.isValid());
}

void tst_QStaticText::textDocumentColor()
{
    QStaticText staticText("A<font color=\"red\">B</font>");
    staticText.setTextFormat(Qt::RichText);
    staticText.prepare();

    QStaticTextPrivate *d = QStaticTextPrivate::get(&staticText);
    QCOMPARE(d->itemCount, 2);

    // The pen should not be marked as dirty when drawing the underline
    QVERIFY(!d->items[0].color.isValid());
    QVERIFY(d->items[1].color.isValid());

    QCOMPARE(d->items[1].color, QColor(Qt::red));
}
#endif

QTEST_MAIN(tst_QStaticText)
#include "tst_qstatictext.moc"
