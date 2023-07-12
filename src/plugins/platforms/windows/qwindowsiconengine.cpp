// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsiconengine.h"

#include <QtCore/qoperatingsystemversion.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpalette.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QWindowsIconEngine::Glyphs QWindowsIconEngine::glyphs() const
{
    if (!QFontInfo(m_iconFont).exactMatch())
        return {};

    static constexpr std::pair<QStringView, Glyphs> glyphMap[] = {
        {u"edit-clear", 0xe894},
        {u"edit-copy", 0xe8c8},
        {u"edit-cut", 0xe8c6},
        {u"edit-delete", 0xe74d},
        {u"edit-find", 0xe721},
        {u"edit-find-replace", Glyphs(0xeb51, 0xeb52)},
        {u"edit-paste", 0xe77f},
        {u"edit-redo", 0xe7a6},
        {u"edit-select-all", 0xe8b3},
        {u"edit-undo", 0xe7a7},
        {u"printer", 0xe749},
        {u"red-heart", Glyphs(0x2764, 0xFE0F)},
        {u"banana", Glyphs(0xffff, 0xD83C, 0xDF4C)},
    };

    const auto it = std::find_if(std::begin(glyphMap), std::end(glyphMap), [this](const auto &c){
        return c.first == m_iconName;
    });
    return it != std::end(glyphMap) ? it->second : Glyphs();
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
    return m_glyphs.isNull();
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
        if (m_glyphs.codepoints[0] == QChar(0xffff)) {
            painter.drawText(rect, Qt::AlignCenter, QString(m_glyphs.codepoints + 1, 2));
        } else {
            for (const auto &glyph : m_glyphs.codepoints) {
                if (glyph.isNull())
                    break;
                painter.drawText(rect, glyph);
            }
        }

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
