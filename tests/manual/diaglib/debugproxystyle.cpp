/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "debugproxystyle.h"

#include <QDebug>
#include <QWidget>
#include <QStyleOption>
#include <QApplication>

#if QT_VERSION < 0x050000
QDebug operator<<(QDebug d, const QPixmap &p)
{
    d << "QPixmap(" << p.size() << ')';
    return d;
}
#endif // QT_VERSION < 0x050000

QDebug operator<<(QDebug debug, const QStyleOption *option)
{
#if QT_VERSION >= 0x050000
    QDebugStateSaver saver(debug);
    debug.nospace();
#endif
    debug << "QStyleOption(";
    if (option)
        debug << "rec=" << option->rect;
    else
        debug << '0';
    debug << ')';
    return debug;
}

namespace QtDiag {

DebugProxyStyle::DebugProxyStyle(QStyle *style) : QProxyStyle(style)
{
#if QT_VERSION >= 0x050000
    const qreal devicePixelRatio = qApp->devicePixelRatio();
#else
    const qreal devicePixelRatio = 1;
#endif
    qDebug() << __FUNCTION__ << QT_VERSION_STR
#if QT_VERSION >= 0x050000
        << QGuiApplication::platformName()
#endif
        << style->objectName() << "devicePixelRatio=" << devicePixelRatio;
}

void DebugProxyStyle::drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    qDebug() << __FUNCTION__ << "element=" << element << option << widget;
    QProxyStyle::drawPrimitive( element, option, painter, widget);
}

void DebugProxyStyle::drawControl(QStyle::ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    qDebug() << __FUNCTION__ << "element=" << element << option << widget;
    QProxyStyle::drawControl(element, option, painter, widget);
}

void DebugProxyStyle::drawComplexControl(QStyle::ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const
{
    qDebug() << __FUNCTION__ << "control=" << control << option << widget;
    QProxyStyle::drawComplexControl(control, option, painter, widget);
}

void DebugProxyStyle::drawItemPixmap(QPainter *painter, const QRect &rect, int alignment, const QPixmap &pixmap) const
{
    qDebug() << __FUNCTION__ << rect << "alignment=" << alignment << pixmap;
    QProxyStyle::drawItemPixmap(painter, rect, alignment, pixmap);
}

QSize DebugProxyStyle::sizeFromContents(QStyle::ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget) const
{
    const QSize result = QProxyStyle::sizeFromContents(type, option, size, widget);
    qDebug() << __FUNCTION__ << size << "type=" << type << option << widget << "returns" << result;
    return result;
}

QRect DebugProxyStyle::subElementRect(QStyle::SubElement element, const QStyleOption *option, const QWidget *widget) const
{
    const QRect result = QProxyStyle::subElementRect(element, option, widget);
    qDebug() << __FUNCTION__ << "element=" << element << option << widget << "returns" << result;
    return result;
}

QRect DebugProxyStyle::subControlRect(QStyle::ComplexControl cc, const QStyleOptionComplex *opt, QStyle::SubControl sc, const QWidget *widget) const
{
    const QRect result = QProxyStyle::subControlRect(cc, opt, sc, widget);
    qDebug() << __FUNCTION__ << "cc=" << cc << "sc=" << sc <<  opt << widget << "returns" << result;
    return result;
}

QRect DebugProxyStyle::itemTextRect(const QFontMetrics &fm, const QRect &r, int flags, bool enabled, const QString &text) const
{
    const QRect result = QProxyStyle::itemTextRect(fm, r, flags, enabled, text);
    qDebug() << __FUNCTION__ <<  r << "flags=" << flags << "enabled=" << enabled
        << text << "returns" << result;
    return result;
}

QRect DebugProxyStyle::itemPixmapRect(const QRect &r, int flags, const QPixmap &pixmap) const
{
    const QRect result = QProxyStyle::itemPixmapRect(r, flags, pixmap);
    qDebug() << __FUNCTION__ << r << "flags=" << flags  << pixmap << "returns" << result;
    return result;
}

int DebugProxyStyle::pixelMetric(QStyle::PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    const int result = QProxyStyle::pixelMetric(metric, option, widget);
    qDebug() << __FUNCTION__ << "metric=" << metric << option << widget << "returns" << result;
    return result;
}

QPixmap DebugProxyStyle::standardPixmap(QStyle::StandardPixmap standardPixmap, const QStyleOption *opt, const QWidget *widget) const
{
    const QPixmap result = QProxyStyle::standardPixmap(standardPixmap, opt, widget);
    qDebug() << __FUNCTION__ << "standardPixmap=" << standardPixmap << opt << "returns" << result;
    return result;
}

QPixmap DebugProxyStyle::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap, const QStyleOption *opt) const
{
    const QPixmap result = QProxyStyle::generatedIconPixmap(iconMode, pixmap, opt);
    qDebug() << __FUNCTION__ << "iconMode=" << iconMode << pixmap << opt << "returns" << result;
    return result;
}

} // namespace QtDiag
