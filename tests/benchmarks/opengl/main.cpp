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
#include <QtGui>
#include <QtOpenGL>

#include <qtest.h>

#include <private/qpaintengine_opengl_p.h>

class OpenGLBench : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void imageDrawing_data();
    void imageDrawing();

    void pathDrawing_data();
    void pathDrawing();

    void painterOverhead();

    void startupCost_data();
    void startupCost();

    void lineDrawing();

    void textDrawing_data();
    void textDrawing();

    void clippedPainting_data();
    void clippedPainting();

    void gradients_data();
    void gradients();

    void textureUpload_data();
    void textureUpload();


private:
    QGLPixelBuffer *pb;
};

void OpenGLBench::initTestCase()
{
    pb = new QGLPixelBuffer(512, 512);

    QPainter p(pb);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::HighQualityAntialiasing);

    p.drawImage(0, 0, QImage(256, 256, QImage::Format_ARGB32_Premultiplied));
}

void OpenGLBench::cleanupTestCase()
{
    delete pb;
}

void OpenGLBench::imageDrawing_data()
{
    QTest::addColumn<bool>("smoothPixmapTransform");
    QTest::addColumn<bool>("highQualityAntialiasing");
    QTest::addColumn<bool>("pixmap");

    for (int i = 0; i < (1 << 3); ++i) {
        bool smoothPixmapTransform = i & 1;
        bool highQualityAntialiasing = i & 2;
        bool pixmap = i & 4;

        QTest::newRow(QString("pixmap=%1 highQualityAntialiasing=%2 smoothPixmapTransform=%3")
                      .arg(pixmap).arg(highQualityAntialiasing).arg(smoothPixmapTransform).toLatin1().data())
            << pixmap << highQualityAntialiasing << smoothPixmapTransform;
    }
}

void OpenGLBench::imageDrawing()
{
    QFETCH(bool, smoothPixmapTransform);
    QFETCH(bool, highQualityAntialiasing);
    QFETCH(bool, pixmap);

    QImage img;
    QPixmap pm;

    if (pixmap)
        pm = QPixmap(800, 800);
    else
        img = QImage(800, 800, QImage::Format_ARGB32_Premultiplied);

    QPainter p(pb);
    p.setRenderHint(QPainter::SmoothPixmapTransform, smoothPixmapTransform);
    p.setRenderHint(QPainter::Antialiasing, highQualityAntialiasing);
    p.setRenderHint(QPainter::HighQualityAntialiasing, highQualityAntialiasing);

    QBENCHMARK {
        if (pixmap) {
            pm.detach();
            p.drawPixmap(0, 0, pm);
        } else {
            img.detach();
            p.drawImage(0, 0, img);
        }
    }
}

Q_DECLARE_METATYPE(QPainterPath)

void OpenGLBench::pathDrawing_data()
{
    QTest::addColumn<QPainterPath>("path");
    QTest::addColumn<bool>("highQualityAntialiasing");

    QList<QPair<QPainterPath, QLatin1String> > paths;

    {
        QPainterPath path;
        path.addRect(-100, -100, 200, 200);
        paths << qMakePair(path, QLatin1String("plain rect"));
    }

    {
        QPainterPath path;
        path.addRoundedRect(-100, -100, 200, 200, 50, 50);
        paths << qMakePair(path, QLatin1String("rounded rect"));
    }

    {
        QPainterPath path;
        path.addEllipse(-100, -100, 200, 200);
        paths << qMakePair(path, QLatin1String("ellipse"));
    }

    for (int j = 0; j < (1 << 1); ++j) {
        bool highQualityAntialiasing = j & 1;

        for (int i = 0; i < paths.size(); ++i) {
            QTest::newRow(QString("path=%1 highQualityAntialiasing=%2")
                    .arg(paths[i].second).arg(highQualityAntialiasing).toLatin1().data())
                << paths[i].first << highQualityAntialiasing;
        }
    }
}

void OpenGLBench::pathDrawing()
{
    QFETCH(QPainterPath, path);
    QFETCH(bool, highQualityAntialiasing);

    // warm-up
    {
        QPainterPath dummy;
        dummy.addRect(-1, -1, 2, 2);
        QPainter p(pb);
        p.setRenderHint(QPainter::Antialiasing, highQualityAntialiasing);
        p.setRenderHint(QPainter::HighQualityAntialiasing, highQualityAntialiasing);
        p.translate(pb->width() / 2, pb->height() / 2);
        p.rotate(30);
        p.drawPath(dummy);
        p.end();
    }

    QPainter p(pb);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);
    p.translate(pb->width() / 2, pb->height() / 2);

    QBENCHMARK {
        p.setRenderHint(QPainter::Antialiasing, highQualityAntialiasing);
        p.setRenderHint(QPainter::HighQualityAntialiasing, highQualityAntialiasing);

        p.rotate(0.01);
        p.drawPath(path);
    }
}

void OpenGLBench::painterOverhead()
{
    QBENCHMARK {
        QPainter p(pb);
    }
}

void OpenGLBench::startupCost_data()
{
    QTest::addColumn<bool>("highQualityAntialiasing");

    QTest::newRow("highQualityAntialiasing=0") << false;
    QTest::newRow("highQualityAntialiasing=1") << true;
}

void OpenGLBench::startupCost()
{
    QFETCH(bool, highQualityAntialiasing);
    QPainterPath path;
    path.addRoundedRect(-100, -100, 200, 200, 20, 20);
    QBENCHMARK {
        QGLPixelBuffer buffer(512, 512);
        QPainter p(&buffer);
        p.setRenderHint(QPainter::Antialiasing, highQualityAntialiasing);
        p.setRenderHint(QPainter::HighQualityAntialiasing, highQualityAntialiasing);

        p.translate(buffer.width() / 2, buffer.height() / 2);
        p.drawPath(path);
    }
}

void OpenGLBench::lineDrawing()
{
    QPainter p(pb);

    QBENCHMARK {
        p.drawLine(10, 10, 500, 500);
    }
}

void OpenGLBench::textDrawing_data()
{
    QTest::addColumn<int>("lines");

    int lines[] = { 1, 2, 4, 8, 16, 32, 64, 128 };

    QTest::newRow("text lines=1 (warmup run)") << 1;
    for (unsigned int i = 0; i < sizeof(lines) / sizeof(int); ++i)
        QTest::newRow(QString("text lines=%0").arg(lines[i]).toLatin1().data()) << lines[i];
}

void OpenGLBench::textDrawing()
{
    QPainter p(pb);

    QFETCH(int, lines);

    p.translate(0, 16);
    QBENCHMARK {
        for (int i = 0; i < lines; ++i)
            p.drawText(0, i, "Hello World!");
    }
}

void OpenGLBench::clippedPainting_data()
{
    QTest::addColumn<QPainterPath>("path");

    QRectF rect = QRectF(0, 0, pb->width(), pb->height()).adjusted(5, 5, -5, -5);

    {
        QPainterPath path;
        path.addRect(rect);
        QTest::newRow("rect path") << path;
    }

    {
        QPainterPath path;
        path.addRoundedRect(rect, 5, 5);
        QTest::newRow("rounded rect path") << path;
    }

    {
        QPainterPath path;
        path.addEllipse(rect);
        QTest::newRow("ellipse path") << path;
    }
}

void OpenGLBench::clippedPainting()
{
    QFETCH(QPainterPath, path);

    QBENCHMARK {
        QPainter p(pb);
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::black);

        p.setClipPath(path);
        p.drawRect(0, 0, pb->width(), pb->height());
    }
}

Q_DECLARE_METATYPE(QGradient::Type)

void OpenGLBench::gradients_data()
{
    QTest::addColumn<QGradient::Type>("gradientType");
    QTest::addColumn<bool>("objectBoundingMode");

    QTest::newRow("warmup run") << QGradient::LinearGradient << false;

    QTest::newRow("linear gradient") << QGradient::LinearGradient << false;
    QTest::newRow("radial gradient") << QGradient::RadialGradient << false;
    QTest::newRow("conical gradient") << QGradient::ConicalGradient << false;

    QTest::newRow("linear gradient, object bounding mode") << QGradient::LinearGradient << true;
    QTest::newRow("radial gradient, object bounding mode") << QGradient::RadialGradient << true;
    QTest::newRow("conical gradient, object bounding mode") << QGradient::ConicalGradient << true;
}

void OpenGLBench::gradients()
{
    QFETCH(QGradient::Type, gradientType);
    QFETCH(bool, objectBoundingMode);

    QPointF a;
    QPointF b = objectBoundingMode ? QPointF(1, 1) : QPointF(pb->width(), pb->height());

    QGradient gradient;
    switch (gradientType) {
    case QGradient::LinearGradient:
        gradient = QLinearGradient(a, b);
        break;
    case QGradient::RadialGradient:
        gradient = QRadialGradient(a, b.x() / 2, b);
        break;
    case QGradient::ConicalGradient:
        gradient = QConicalGradient((a + b)/2, 0);
        break;
    default:
        break;
    }

    if (objectBoundingMode)
        gradient.setCoordinateMode(QGradient::ObjectBoundingMode);

    gradient.setColorAt(0, Qt::red);
    gradient.setColorAt(0.2, Qt::blue);
    gradient.setColorAt(0.4, Qt::transparent);
    gradient.setColorAt(0.6, Qt::green);
    gradient.setColorAt(0.8, Qt::black);
    gradient.setColorAt(1, Qt::white);

    QPainter p(pb);

    QBENCHMARK {
        p.fillRect(0, 0, pb->width(), pb->height(), gradient);
        glFinish();
    }
}

void OpenGLBench::textureUpload_data()
{
    QTest::addColumn<int>("size");
    QTest::addColumn<int>("flags");
    QTest::addColumn<int>("format");

    int sizes[] = { 8, 10, 16, 20, 32, 50, 64, 100, 128, 200, 256, 500, 512, 1000, 1024, 2000, 2048, -1 };
    int flags[] = { QGLContext::InternalBindOption,
                  QGLContext::DefaultBindOption,
                  -1 };
    int formats[] = { GL_RGB, GL_RGBA, -1 };

    for (int s = 0; sizes[s] != -1; ++s) {
        for (int f = 0; flags[f] != -1; ++f) {
            for (int a = 0; formats[a] != -1; ++a) {
                QByteArray name;
                name.append("size=").append(QByteArray::number(sizes[s]));
                name.append(", flags=").append(f == 0 ? "internal" : "default");
                name.append(", format=").append(a == 0 ? "RGB" : "RGBA");
                QTest::newRow(name.constData()) << sizes[s] << flags[f] << formats[a];
            }
        }
    }
}

void OpenGLBench::textureUpload()
{
    QFETCH(int, size);
    QFETCH(int, flags);
    QFETCH(int, format);

    QPixmap pixmap(size, size);

    if (format == GL_RGB)
        pixmap.fill(Qt::red);
    else
        pixmap.fill(Qt::transparent);

    pb->makeCurrent();
    QGLContext *context = const_cast<QGLContext *>(QGLContext::currentContext());
    QTime time;

    time.start();
    context->bindTexture(pixmap, GL_TEXTURE_2D, format, (QGLContext::BindOptions) flags);
    QTest::setBenchmarkResult(time.elapsed(), QTest::WalltimeMilliseconds);
}

QTEST_MAIN(OpenGLBench)

#include "main.moc"
