/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the FOO module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef BENCHMARKTESTS_H
#define BENCHMARKTESTS_H

#include <QApplication>
#include <QTextDocument>
#include <QDesktopWidget>
#include <QTextLayout>
#include <QFontMetrics>
#include <QDebug>
#include <QStaticText>
#include <QPainter>

class Benchmark
{
public:
    virtual ~Benchmark() {}

    Benchmark(const QSize &size)
            : m_size(size)
    {
        for (int i=0; i<16; ++i) {
            m_colors[i] = QColor::fromRgbF((rand() % 4) / 3.0,
                                           (rand() % 4) / 3.0,
                                           (rand() % 4) / 3.0,
                                           1);
        }
    }

    virtual void draw(QPainter *p, const QRect &rect, int iteration) = 0;
    virtual QString name() const = 0;

    inline const QSize &size() const
    {
        return m_size;
    }
    virtual void begin(QPainter *, int iterations = 1) { Q_UNUSED(iterations); }
    virtual void end(QPainter *) { }

    inline const QColor &randomColor(int i) { return m_colors[i % 16]; }

protected:
    QColor m_colors[16];
    QSize m_size;
};

class PaintingRectAdjuster
{
public:
    PaintingRectAdjuster()
            : m_benchmark(0),
            m_bounds(),
            m_screen_filled(false)
    {
    }

    const QRect &newPaintingRect() {
        m_rect.translate(m_rect.width(), 0);

        if (m_rect.right() > m_bounds.width()) {
            m_rect.moveLeft(m_bounds.left());
            m_rect.translate(0,m_rect.height());
            if (m_rect.bottom() > m_bounds.height()) {
                m_screen_filled = true;
                m_rect.moveTo(m_bounds.topLeft());
            }
        }
        return m_rect;
    }

    inline bool isScreenFilled() const
    { return m_screen_filled; }

    void reset(const QRect &bounds)
    {
        m_bounds = bounds;
        m_rect.moveTo(m_bounds.topLeft());
        m_rect = QRect(m_bounds.topLeft(),m_benchmark->size());
        m_rect.translate(-m_rect.width(),0);
        m_screen_filled = false;
    }

    inline void setNewBenchmark( Benchmark *benchmark )
    {
        m_benchmark = benchmark;
    }

protected:
    Benchmark *m_benchmark;
    QRect m_rect;
    QRect m_bounds;
    bool m_screen_filled;
};

class FillRectBenchmark : public Benchmark
{
public:
    FillRectBenchmark(int size)
        : Benchmark(QSize(size, size))
    {
    }

    virtual void draw(QPainter *p, const QRect &rect, int iterationCount) {
        p->fillRect(rect, randomColor(iterationCount));
    }

    virtual QString name() const {
        return QString::fromLatin1("fillRect(%1)").arg(m_size.width());
   }
};

class ImageFillRectBenchmark : public Benchmark
{
public:
    ImageFillRectBenchmark(int size)
        : Benchmark(QSize(size, size))
    {
        int s = rand() % 24 + 8;
        m_content = QImage(s, s, QImage::Format_ARGB32_Premultiplied);
        QPainter p(&m_content);
        p.fillRect(0, 0, s, s, Qt::white);
        p.fillRect(s/2, 0, s/2, s/2, Qt::gray);
        p.fillRect(0, s/2, s/2, s/2, Qt::gray);
        p.end();

        m_brush = QBrush(m_content);
    }

    virtual void draw(QPainter *p, const QRect &rect, int) {
        p->fillRect(rect, m_brush);
    }

    virtual QString name() const {
        return QString::fromLatin1("fillRect with image(%1)").arg(m_size.width());
   }

private:
    QImage m_content;
    QBrush m_brush;
};


class DrawRectBenchmark : public Benchmark
{
public:
    DrawRectBenchmark(int size)
        : Benchmark(QSize(size, size))
    {
    }

    virtual void begin(QPainter *p, int) {
        p->setPen(Qt::NoPen);
        p->setBrush(randomColor(m_size.width()));
    }


    virtual void draw(QPainter *p, const QRect &rect, int) {
        p->drawRect(rect);
    }

    virtual QString name() const {
        return QString::fromLatin1("drawRect(%1)").arg(m_size.width());
   }
};


class DrawRectWithBrushChangeBenchmark : public Benchmark
{
public:
    DrawRectWithBrushChangeBenchmark(int size)
        : Benchmark(QSize(size, size))
    {
    }

    virtual void begin(QPainter *p, int) {
        p->setPen(Qt::NoPen);
    }


    virtual void draw(QPainter *p, const QRect &rect, int i) {
        p->setBrush(randomColor(i));
        p->drawRect(rect);
    }

    virtual QString name() const {
        return QString::fromLatin1("drawRect with brushchange(%1)").arg(m_size.width());
   }
};

class RoundRectBenchmark : public Benchmark
{
public:
    RoundRectBenchmark(int size)
        : Benchmark(QSize(size, size))
    {
        m_roundness = size / 4.;
    }

    virtual void begin(QPainter *p, int) {
        p->setPen(Qt::NoPen);
        p->setBrush(Qt::red);
    }

    virtual void draw(QPainter *p, const QRect &rect, int) {
        p->drawRoundedRect(rect, m_roundness, m_roundness);
    }

    virtual QString name() const {
        return QString::fromLatin1("drawRoundedRect(%1)").arg(m_size.width());
    }

    qreal m_roundness;
};


class ArcsBenchmark : public Benchmark
{
public:
    enum Type {
        Stroked         = 0x0001,
        Filled          = 0x0002,

        ArcShape        = 0x0010,
        ChordShape      = 0x0020,
        PieShape        = 0x0040,
        CircleShape     = 0x0080,
        Shapes          = 0x00f0

    };

    ArcsBenchmark(int size, uint type)
        : Benchmark(QSize(size, size)),
          m_type(type)
    {
    }

    virtual void begin(QPainter *p, int) {
        if (m_type & Stroked)
            p->setPen(Qt::black);
        else
            p->setPen(Qt::NoPen);

        if (m_type & Filled)
            p->setBrush(Qt::red);
        else
            p->setBrush(Qt::NoBrush);
    }

    virtual void draw(QPainter *p, const QRect &rect, int) {
        switch (m_type & Shapes) {
        case ArcShape:
            p->drawArc(rect, 45*16, 120*16);
            break;
        case ChordShape:
            p->drawChord(rect, 45*16, 120*16);
            break;
        case PieShape:
            p->drawPie(rect, 45*16, 120*16);
            break;
        case CircleShape:
            p->drawEllipse(rect);
            break;
        }
    }

    virtual QString name() const {
        QString fillStroke;

        if ((m_type & (Stroked|Filled)) == (Stroked|Filled)) {
            fillStroke = QLatin1String("Fill & Outline");
        } else if (m_type & Stroked) {
            fillStroke = QLatin1String("Outline");
        } else if (m_type & Filled) {
            fillStroke = QLatin1String("Fill");
        }

        QString shape;
        if (m_type & PieShape) shape = QLatin1String("drawPie");
        else if (m_type & ChordShape) shape = QLatin1String("drawChord");
        else if (m_type & ArcShape) shape = QLatin1String("drawArc");
        else if (m_type & CircleShape) shape = QLatin1String("drawEllipse");

        return QString::fromLatin1("%1(%2) %3").arg(shape).arg(m_size.width()).arg(fillStroke);
    }

    uint m_type;
};


class DrawScaledImage : public Benchmark
{
public:
    DrawScaledImage(const QImage &image, qreal scale, bool asPixmap)
        : Benchmark(QSize(image.width(), image.height())),
          m_image(image),
          m_type(m_as_pixmap ? "Pixmap" : "Image"),
          m_scale(scale),
          m_as_pixmap(asPixmap)
    {
        m_pixmap = QPixmap::fromImage(m_image);
    }
    DrawScaledImage(const QString& type, const QPixmap &pixmap, qreal scale)
        : Benchmark(QSize(pixmap.width(), pixmap.height())),
          m_type(type),
          m_scale(scale),
          m_as_pixmap(true),
          m_pixmap(pixmap)
    {
    }

    virtual void begin(QPainter *p, int) {
        p->scale(m_scale, m_scale);
    }

    virtual void draw(QPainter *p, const QRect &rect, int) {
        if (m_as_pixmap)
            p->drawPixmap(rect.topLeft(), m_pixmap);
        else
            p->drawImage(rect.topLeft(), m_image);
    }

    virtual QString name() const {
        return QString::fromLatin1("draw%4(%1) at scale=%2, depth=%3")
            .arg(m_size.width())
            .arg(m_scale)
            .arg(m_as_pixmap ? m_pixmap.depth() : m_image.depth())
            .arg(m_type);
   }

private:
    QImage m_image;
    QString m_type;
    qreal m_scale;
    bool m_as_pixmap;
    QPixmap m_pixmap;
};

class DrawTransformedImage : public Benchmark
{
public:
    DrawTransformedImage(const QImage &image, bool asPixmap)
        : Benchmark(QSize(image.width(), image.height())),
          m_image(image),
          m_type(m_as_pixmap ? "Pixmap" : "Image"),
          m_as_pixmap(asPixmap)
    {
        m_pixmap = QPixmap::fromImage(m_image);
    }
    DrawTransformedImage(const QString& type, const QPixmap &pixmap)
        : Benchmark(QSize(pixmap.width(), pixmap.height())),
          m_type(type),
          m_as_pixmap(true),
          m_pixmap(pixmap)
    {
    }

    virtual void draw(QPainter *p, const QRect &rect, int) {
        QTransform oldTransform = p->transform();
        p->translate(0.5 * rect.width() + rect.left(), 0.5 * rect.height() + rect.top());
        p->shear(0.25, 0.0);
        p->rotate(5.0);
        if (m_as_pixmap)
            p->drawPixmap(-0.5 * rect.width(), -0.5 * rect.height(), m_pixmap);
        else
            p->drawImage(-0.5 * rect.width(), -0.5 * rect.height(), m_image);
        p->setTransform(oldTransform);
    }

    virtual QString name() const {
        return QString::fromLatin1("draw%3(%1) w/transform, depth=%2")
            .arg(m_size.width())
            .arg(m_as_pixmap ? m_pixmap.depth() : m_image.depth())
            .arg(m_type);
   }

private:
    QImage m_image;
    QString m_type;
    bool m_as_pixmap;
    QPixmap m_pixmap;
};


class DrawImage : public Benchmark
{
public:
    DrawImage(const QImage &image, bool asPixmap)
        : Benchmark(QSize(image.width(), image.height())),
          m_image(image),
          m_type(m_as_pixmap ? "Pixmap" : "Image"),
          m_as_pixmap(asPixmap)
    {
        m_pixmap = QPixmap::fromImage(image);
    }
    DrawImage(const QString& type, const QPixmap &pixmap)
        : Benchmark(QSize(pixmap.width(), pixmap.height())),
          m_type(type),
          m_as_pixmap(true),
          m_pixmap(pixmap)
    {
    }

    virtual void draw(QPainter *p, const QRect &rect, int) {
        if (m_as_pixmap)
            p->drawPixmap(rect.topLeft(), m_pixmap);
        else
            p->drawImage(rect.topLeft(), m_image);
    }

    virtual QString name() const {
        return QString::fromLatin1("draw%2(%1), depth=%3")
            .arg(m_size.width())
            .arg(m_type)
            .arg(m_as_pixmap ? m_pixmap.depth() : m_image.depth());
   }

private:
    QImage m_image;
    QString m_type;
    bool m_as_pixmap;
    QPixmap m_pixmap;
};


class DrawText : public Benchmark
{
public:
    enum Mode {
        PainterMode,
        PainterQPointMode,
        LayoutMode,
        DocumentMode,
        PixmapMode,
        StaticTextMode,
        StaticTextWithMaximumSizeMode,
        StaticTextBackendOptimizations
    };

    DrawText(const QString &text, Mode mode)
        : Benchmark(QSize()), m_mode(mode), m_text(text), m_document(text), m_layout(text)
    {
    }

    virtual void begin(QPainter *p, int iterations) {
        m_staticTexts.clear();
        m_currentStaticText = 0;
        m_pixmaps.clear();
        m_currentPixmap = 0;
        QRect m_bounds = QRect(0,0,p->device()->width(), p->device()->height());
        switch (m_mode) {
        case PainterMode:
            m_size = (p->boundingRect(m_bounds, 0, m_text)).size();
//            m_rect = m_rect.translated(-m_rect.topLeft());
            break;
        case DocumentMode:
            m_size = QSize(m_document.size().toSize());
            break;
        case PixmapMode:
            for (int i=0; i<4; ++i) {
                m_size = (p->boundingRect(m_bounds, 0, m_text)).size();
                QPixmap pixmap = QPixmap(m_size);
                pixmap.fill(Qt::transparent);
                {
                    QPainter p(&pixmap);
                    p.drawText(pixmap.rect(), m_text);
                }
                m_pixmaps.append(pixmap);
            }
            break;

        case LayoutMode: {
            QRect r = p->boundingRect(m_bounds, 0, m_text);
            QStringList lines = m_text.split('\n');
            int height = 0;
            int leading = p->fontMetrics().leading();
            m_layout.beginLayout();
            for (int i=0; i<lines.size(); ++i) {
                QTextLine textLine = m_layout.createLine();
                if (textLine.isValid()) {
                    textLine.setLineWidth(r.width());
                    textLine.setPosition(QPointF(0, height));
                    height += leading + textLine.height();
                }
            }
            m_layout.endLayout();
            m_layout.setCacheEnabled(true);
            m_size = m_layout.boundingRect().toRect().size();
            break; }

        case StaticTextWithMaximumSizeMode: {
            QStaticText staticText;
            m_size = (p->boundingRect(m_bounds, 0, m_text)).size();
            staticText.setTextWidth(m_size.width() + 10);
            staticText.setText(m_text);
            staticText.prepare(p->transform(), p->font());
            m_staticTexts.append(staticText);
            break;
        }
        case StaticTextBackendOptimizations: {
            m_size = (p->boundingRect(m_bounds, 0, m_text)).size();
            for (int i=0; i<iterations; ++i) {
                QStaticText staticText;
                staticText.setPerformanceHint(QStaticText::AggressiveCaching);
                staticText.setTextWidth(m_size.width() + 10);
                staticText.setText(m_text);
                staticText.prepare(p->transform(), p->font());
                m_staticTexts.append(staticText);
            }

            break;
        }
        case StaticTextMode: {
            QStaticText staticText;
            staticText.setText(m_text);
            staticText.prepare(p->transform(), p->font());
            m_staticTexts.append(staticText);

            QFontMetrics fm(p->font());
            m_size = QSize(fm.width(m_text, m_text.length()), fm.height());

            break;
        }
        case PainterQPointMode: {
            QFontMetrics fm(p->font());
            m_size = QSize(fm.width(m_text, m_text.length()), fm.height());
            break;
        }

        }
    }

    virtual void draw(QPainter *p, const QRect &rect, int)
    {
        switch (m_mode) {
        case PainterMode:
            p->drawText(rect, 0, m_text);
            break;
        case PainterQPointMode:
            p->drawText(rect.topLeft(), m_text);
            break;
        case PixmapMode:
            p->drawPixmap(rect.topLeft(), m_pixmaps.at(m_currentPixmap));
            m_currentPixmap = (m_currentPixmap + 1) % m_pixmaps.size();
            break;
        case DocumentMode:
            p->translate(rect.topLeft());
            m_document.drawContents(p);
            p->translate(-rect.topLeft());
            break;
        case LayoutMode:
            m_layout.draw(p, rect.topLeft());
            break;
        case StaticTextWithMaximumSizeMode:
        case StaticTextMode:
            p->drawStaticText(rect.topLeft(), m_staticTexts.at(0));
            break;
        case StaticTextBackendOptimizations:
            p->drawStaticText(rect.topLeft(), m_staticTexts.at(m_currentStaticText));
            m_currentStaticText = (m_currentStaticText + 1) % m_staticTexts.size();
            break;
        }
    }

    virtual QString name() const {
        int letters = m_text.length();
        int lines = m_text.count('\n');
        if (lines == 0)
            lines = 1;
        QString type;
        switch (m_mode) {
        case PainterMode: type = "drawText(rect)"; break;
        case PainterQPointMode: type = "drawText(point)"; break;
        case LayoutMode: type = "layout.draw()"; break;
        case DocumentMode: type = "doc.drawContents()"; break;
        case PixmapMode: type = "pixmap cached text"; break;
        case StaticTextMode: type = "drawStaticText()"; break;
        case StaticTextWithMaximumSizeMode: type = "drawStaticText() w/ maxsize"; break;
        case StaticTextBackendOptimizations: type = "drawStaticText() w/ backend optimizations"; break;
        }

        return QString::fromLatin1("%3, len=%1, lines=%2")
            .arg(letters)
            .arg(lines)
            .arg(type);
    }

private:
    Mode m_mode;
    QString m_text;
    QTextDocument m_document;
    QTextLayout m_layout;

    QList<QPixmap> m_pixmaps;
    int m_currentPixmap;

    int m_currentStaticText;
    QList<QStaticText> m_staticTexts;
};

class ClippedDrawRectBenchmark : public Benchmark
{
public:
    enum ClipType {
        RectClip,
        TwoRectRegionClip,
        EllipseRegionClip,
        TwoRectPathClip,
        EllipsePathClip,
        AAEllipsePathClip,
        EllipseRegionThenRectClip,
        EllipsePathThenRectClip
    };

    ClippedDrawRectBenchmark(int size, ClipType type)
        : Benchmark(QSize(size, size)), m_type(type)
    {
    }

    virtual void begin(QPainter *p, int) {
        QRect m_bounds = QRect(0,0,p->device()->width(), p->device()->height());
        p->setPen(Qt::NoPen);
        p->setBrush(Qt::red);

        switch (m_type) {
        case RectClip:
            p->setClipRect(m_bounds.adjusted(1, 1, -1, -1));
            break;
        case TwoRectRegionClip:
            p->setClipRegion(QRegion(m_bounds.adjusted(0, 0, -1, -1))
                             | QRegion(m_bounds.adjusted(1, 1, 0, 0)));
            break;
        case EllipseRegionClip:
            p->setClipRegion(QRegion(m_bounds, QRegion::Ellipse));
            break;
        case TwoRectPathClip:
            {
                QPainterPath path;
                path.addRect(m_bounds.adjusted(0, 0, -1, -1));
                path.addRect(m_bounds.adjusted(1, 1, 0, 0));
                path.setFillRule(Qt::WindingFill);
                p->setClipPath(path);
            }
            break;
        case EllipsePathClip:
            {
                QPainterPath path;
                path.addEllipse(m_bounds);
                p->setClipPath(path);
            }
            break;
        case AAEllipsePathClip:
            {
                QPainterPath path;
                path.addEllipse(m_bounds);
                p->setRenderHint(QPainter::Antialiasing);
                p->setClipPath(path);
                p->setRenderHint(QPainter::Antialiasing, false);
            }
            break;
        case EllipseRegionThenRectClip:
            p->setClipRegion(QRegion(m_bounds, QRegion::Ellipse));
            p->setClipRegion(QRegion(m_bounds.width() / 4,
                                   m_bounds.height() / 4,
                                   m_bounds.width() / 2,
                                   m_bounds.height() / 2), Qt::IntersectClip);
            break;
        case EllipsePathThenRectClip:
            {
                QPainterPath path;
                path.addEllipse(m_bounds);
                p->setClipPath(path);
                p->setClipRegion(QRegion(m_bounds.width() / 4,
                                         m_bounds.height() / 4,
                                         m_bounds.width() / 2,
                                         m_bounds.height() / 2), Qt::IntersectClip);
            }
            break;
        }
    }

    virtual void draw(QPainter *p, const QRect &rect, int) {
        p->drawRect(rect);
    }

    virtual QString name() const {
        QString namedType;
        switch (m_type) {
        case RectClip:
            namedType = "rect";
            break;
        case TwoRectRegionClip:
            namedType = "two-rect-region";
            break;
        case EllipseRegionClip:
            namedType = "ellipse-region";
            break;
        case TwoRectPathClip:
            namedType = "two-rect-path";
            break;
        case EllipsePathClip:
            namedType = "ellipse-path";
            break;
        case AAEllipsePathClip:
            namedType = "aa-ellipse-path";
            break;
        case EllipseRegionThenRectClip:
            namedType = "ellipseregion&rect";
            break;
        case EllipsePathThenRectClip:
            namedType = "ellipsepath&rect";
            break;
        }
        return QString::fromLatin1("%1-clipped-drawRect(%2)").arg(namedType).arg(m_size.width());
   }

    ClipType m_type;
};

class LinesBenchmark : public Benchmark
{
public:
    enum LineType {
        Horizontal_Integer,
        Diagonal_Integer,
        Vertical_Integer,
        Horizontal_Float,
        Diagonal_Float,
        Vertical_Float
    };

    LinesBenchmark(int length, LineType type)
        : Benchmark(QSize(qAbs(length), qAbs(length))),
          m_type(type),
          m_length(length)
    {

    }

    virtual void draw(QPainter *p, const QRect &rect, int) {
        switch (m_type) {
        case Horizontal_Integer:
            p->drawLine(QLine(rect.x(), rect.y(), rect.x() + m_length, rect.y()));
            break;
        case Diagonal_Integer:
            p->drawLine(QLine(rect.x(), rect.y(), rect.x() + m_length, rect.y() + m_length));
            break;
        case Vertical_Integer:
            p->drawLine(QLine(rect.x() + 4, rect.y(), rect.x() + 4, rect.y() + m_length));
            break;
        case Horizontal_Float:
            p->drawLine(QLineF(rect.x(), rect.y(), rect.x() + m_length, rect.y()));
            break;
        case Diagonal_Float:
            p->drawLine(QLineF(rect.x(), rect.y(), rect.x() + m_length, rect.y() + m_length));
            break;
        case Vertical_Float:
            p->drawLine(QLineF(rect.x() + 4, rect.y(), rect.x() + 4, rect.y() + m_length));
            break;
        }
    }

    virtual QString name() const {
        const char *names[] = {
            "Hor_I",
            "Diag_I",
            "Ver_I",
            "Hor_F",
            "Diag_F",
            "Ver_F"
        };
        return QString::fromLatin1("drawLine(size=%1,type=%2)").arg(m_length).arg(names[m_type]);
    }

    LineType m_type;
    int m_length;
};

#endif // BENCHMARKTESTS_H
