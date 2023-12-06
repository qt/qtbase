// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsiconengine.h"

#include <QtCore/qoperatingsystemversion.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpalette.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QString QWindowsIconEngine::glyphs() const
{
    if (!QFontInfo(m_iconFont).exactMatch())
        return {};

    static constexpr std::pair<QLatin1StringView, QStringView> glyphMap[] = {
        {QIcon::ThemeIcon::EditClear, u"\ue894"},
        {QIcon::ThemeIcon::EditCopy, u"\ue8c8"},
        {QIcon::ThemeIcon::EditCut, u"\ue8c6"},
        {QIcon::ThemeIcon::EditDelete, u"\ue74d"},
        {QIcon::ThemeIcon::EditFind, u"\ue721"},
        {QIcon::ThemeIcon::EditPaste, u"\ue77f"},
        {QIcon::ThemeIcon::EditRedo, u"\ue7a6"},
        {QIcon::ThemeIcon::EditSelectAll, u"\ue8b3"},
        {QIcon::ThemeIcon::EditUndo, u"\ue7a7"},
        {QIcon::ThemeIcon::Printer, u"\ue749"},
        {QLatin1StringView("banana"), u"🍌"},
    };

    const auto it = std::find_if(std::begin(glyphMap), std::end(glyphMap), [this](const auto &c){
        return c.first == m_iconName;
    });

    return it != std::end(glyphMap) ? it->second.toString()
                                    : (m_iconName.length() == 1 ? m_iconName : QString());
}

namespace {
auto iconFontFamily()
{
    static const bool isWindows11 = QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows11;
    return isWindows11 ? u"Segoe Fluent Icons"_s
                       : u"Segoe MDL2 Assets"_s;
}
}

QWindowsIconEngine::QWindowsIconEngine(const QString &iconName)
    : m_iconName(iconName), m_iconFont(iconFontFamily())
    , m_glyphs(glyphs())
{
}

QWindowsIconEngine::~QWindowsIconEngine()
{}

QIconEngine *QWindowsIconEngine::clone() const
{
    return new QWindowsIconEngine(m_iconName);
}

QString QWindowsIconEngine::key() const
{
    return u"QWindowsIconEngine"_s;
}

QString QWindowsIconEngine::iconName()
{
    return m_iconName;
}

bool QWindowsIconEngine::isNull()
{
    if (m_glyphs.isEmpty())
        return true;

    const QChar c0 = m_glyphs.at(0);
    const QFontMetrics fontMetrics(m_iconFont);
    if (c0.category() == QChar::Other_Surrogate && m_glyphs.size() > 1)
        return !fontMetrics.inFontUcs4(QChar::surrogateToUcs4(c0, m_glyphs.at(1)));
    return !fontMetrics.inFont(c0);
}

QList<QSize> QWindowsIconEngine::availableSizes(QIcon::Mode, QIcon::State)
{
    return {{16, 16}, {24, 24}, {48, 48}, {128, 128}};
}

QSize QWindowsIconEngine::actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    return QIconEngine::actualSize(size, mode, state);
}

QPixmap QWindowsIconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    return scaledPixmap(size, mode, state, 1.0);
}

QPixmap QWindowsIconEngine::scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale)
{
    const quint64 cacheKey = calculateCacheKey(mode, state);
    if (cacheKey != m_cacheKey || m_pixmap.size() != size || m_pixmap.devicePixelRatio() != scale) {
        m_pixmap = QPixmap(size * scale);
        m_pixmap.fill(Qt::transparent);
        m_pixmap.setDevicePixelRatio(scale);

        QPainter painter(&m_pixmap);
        QFont renderFont(m_iconFont);
        renderFont.setPixelSize(size.height());
        painter.setFont(renderFont);

        QPalette palette;
        switch (mode) {
        case QIcon::Active:
            painter.setPen(palette.color(QPalette::Active, QPalette::Text));
            break;
        case QIcon::Normal:
            painter.setPen(palette.color(QPalette::Active, QPalette::Text));
            break;
        case QIcon::Disabled:
            painter.setPen(palette.color(QPalette::Disabled, QPalette::Text));
            break;
        case QIcon::Selected:
            painter.setPen(palette.color(QPalette::Active, QPalette::HighlightedText));
            break;
        }

        const QRect rect({0, 0}, size);
        painter.drawText(rect, Qt::AlignCenter, m_glyphs);

        m_cacheKey = cacheKey;
    }

    return m_pixmap;
}

void QWindowsIconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state)
{
    const qreal scale = painter->device()->devicePixelRatio();
    painter->drawPixmap(rect, scaledPixmap(rect.size(), mode, state, scale));
}

QT_END_NAMESPACE
