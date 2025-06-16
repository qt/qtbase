// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qfonticonengine_p.h"

#ifndef QT_NO_ICON

#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>
#include <QtCore/qset.h>

#include <QtGui/qfontdatabase.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpainterpath.h>
#include <QtGui/qpalette.h>
#include <QtGui/qtextlayout.h>

#include <QtGui/private/qfont_p.h>
#include <QtGui/private/qfontengine_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QFontIconEngine::QFontIconEngine(const QString &iconName, const QFont &font)
    : m_iconName(iconName)
    , m_iconFont(font)
{
    Q_ASSERT_X(font.styleStrategy() & QFont::NoFontMerging, "QFontIconEngine",
                "Icon fonts must not use font merging");
}

QFontIconEngine::~QFontIconEngine() = default;

QIconEngine *QFontIconEngine::clone() const
{
    return new QFontIconEngine(m_iconName, m_iconFont);
}

QString QFontIconEngine::key() const
{
    return u"QFontIconEngine("_s + m_iconFont.key() + u')';
}

QString QFontIconEngine::iconName()
{
    return m_iconName;
}

bool QFontIconEngine::isNull()
{
    const QString text = string();
    if (!text.isEmpty()) {
        const QChar c0 = text.at(0);
        const QFontMetrics fontMetrics(m_iconFont);
        if (c0.isHighSurrogate() && text.size() > 1)
            return !fontMetrics.inFontUcs4(QChar::surrogateToUcs4(c0, text.at(1)));
        return !fontMetrics.inFont(c0);
    }

    return glyph() == 0;
}

QList<QSize> QFontIconEngine::availableSizes(QIcon::Mode, QIcon::State)
{
    return {{16, 16}, {24, 24}, {48, 48}, {128, 128}};
}

QSize QFontIconEngine::actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    if (isNull())
        return QIconEngine::actualSize(size, mode, state);

    QFont renderFont(m_iconFont);
    renderFont.setPixelSize(size.height());
    QSizeF result;
    if (const glyph_t glyphIndex = glyph()) {
        QFontEngine *engine = QFontPrivate::get(renderFont)->engineForScript(QChar::Script_Common);

        const glyph_metrics_t gm = engine->boundingBox(glyphIndex);
        const qreal glyph_x = gm.x.toReal();
        const qreal glyph_y = gm.y.toReal();
        const qreal glyph_width = (gm.x + gm.width).toReal() - glyph_x;
        const qreal glyph_height = (gm.y + gm.height).toReal() - glyph_y;

        if (glyph_width > .0 && glyph_height > .0)
            result = {glyph_width, glyph_height};
    } else if (const QString text = string(); !text.isEmpty()) {
        const QFontMetricsF fm(renderFont);
        result = {fm.horizontalAdvance(text), fm.tightBoundingRect(text).height()};
    }
    if (!result.isValid())
        return QIconEngine::actualSize(size, mode, state);

    return result.scaled(size, Qt::KeepAspectRatio).toSize();
}

QPixmap QFontIconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    return scaledPixmap(size, mode, state, 1.0);
}

QPixmap QFontIconEngine::scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale)
{
    const quint64 cacheKey = calculateCacheKey(mode, state);
    const QSize fittingSize = actualSize(size, mode, state);
    if (cacheKey != m_pixmapCacheKey || m_pixmap.deviceIndependentSize() != fittingSize
     || m_pixmap.devicePixelRatio() != scale) {
        m_pixmap = QPixmap(fittingSize * scale);
        m_pixmap.fill(Qt::transparent);
        m_pixmap.setDevicePixelRatio(scale);

        if (!m_pixmap.isNull()) {
            QPainter painter(&m_pixmap);
            paint(&painter, QRect(QPoint(), fittingSize), mode, state);
        }

        m_pixmapCacheKey = cacheKey;
    }

    return m_pixmap;
}

void QFontIconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state)
{
    Q_UNUSED(state);

    painter->save();
    QFont renderFont(m_iconFont);
    renderFont.setPixelSize(rect.height());

    QColor color = Qt::black;
    QPalette palette;
    switch (mode) {
    case QIcon::Active:
        color = palette.color(QPalette::Active, QPalette::Text);
        break;
    case QIcon::Normal:
        color = palette.color(QPalette::Active, QPalette::Text);
        break;
    case QIcon::Disabled:
        color = palette.color(QPalette::Disabled, QPalette::Text);
        break;
    case QIcon::Selected:
        color = palette.color(QPalette::Active, QPalette::HighlightedText);
        break;
    }

    if (glyph_t glyphIndex = glyph()) {
        QFontEngine *engine = QFontPrivate::get(renderFont)->engineForScript(QChar::Script_Common);

        const glyph_metrics_t gm = engine->boundingBox(glyphIndex);
        const int glyph_x = qFloor(gm.x.toReal());
        const int glyph_y = qFloor(gm.y.toReal());
        const int glyph_width = qCeil((gm.x + gm.width).toReal()) - glyph_x;
        const int glyph_height = qCeil((gm.y + gm.height).toReal()) - glyph_y;

        if (glyph_width > 0 && glyph_height > 0) {
            QFixedPoint pt(QFixed(-glyph_x), QFixed(-glyph_y));
            QPainterPath path;
            path.setFillRule(Qt::WindingFill);
            engine->addGlyphsToPath(&glyphIndex, &pt, 1, &path, {});
            // make the glyph fit tightly into rect
            const QRectF pathBoundingRect = path.boundingRect();
            // center the glyph inside the rect
            const QPointF topLeft = rect.topLeft() - pathBoundingRect.topLeft()
                            + (QPointF(rect.width(), rect.height())
                                - QPointF(pathBoundingRect.width(), pathBoundingRect.height())) / 2;
            painter->translate(topLeft);

            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(color);
            painter->drawPath(path);
        }
    } else if (const QString text = string(); !text.isEmpty()) {
        painter->setFont(renderFont);
        painter->setPen(color);
        painter->drawText(rect, Qt::AlignCenter, text);
    }
    painter->restore();
}

QString QFontIconEngine::string() const
{
    return {};
}

glyph_t QFontIconEngine::glyph() const
{
    if (m_glyph == uninitializedGlyph) {
        QFontEngine *engine = QFontPrivate::get(m_iconFont)->engineForScript(QChar::Script_Common);
        if (engine)
            m_glyph = engine->findGlyph(QLatin1StringView(m_iconName.toLatin1()));
        if (!m_glyph) {
            // May not be a named glyph, but there might be a ligature for the
            // icon name.
            QTextLayout layout(m_iconName, m_iconFont);
            layout.beginLayout();
            layout.createLine();
            layout.endLayout();
            const auto glyphRuns = layout.glyphRuns();
            if (glyphRuns.size() == 1) {
                const auto glyphIndexes = glyphRuns.first().glyphIndexes();
                if (glyphIndexes.size() == 1)
                    m_glyph = glyphIndexes.first();
            }
        }
    }
    return m_glyph;
}

QT_END_NAMESPACE

#endif // QT_NO_ICON
