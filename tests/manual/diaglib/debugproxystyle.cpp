// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debugproxystyle.h"
#include "eventfilter.h"

#include <QDebug>
#include <QWidget>
#include <QStyleOption>
#include <QApplication>

QDebug operator<<(QDebug debug, const QStyleOption *option)
{
    QDebugStateSaver saver(debug);
    debug.noquote();
    debug.nospace();
    if (!option) {
        debug << "QStyleOption(0)";
        return debug;
    }
    if (const QStyleOptionViewItem *ivo = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
        debug << "QStyleOptionViewItem(";
        debug << ivo->index;
        if (const int textSize = ivo->text.size())
            debug << ", \"" << (textSize < 20 ? ivo->text : ivo->text.left(20) + QLatin1String("...")) << '"';
        debug << ", ";
    } else {
        debug << "QStyleOption(";
    }
    debug << "rect=" << option->rect.width() << 'x' << option->rect.height() << Qt::forcesign
          << option->rect.x() << option->rect.y() << Qt::noforcesign;
    if (option->state != QStyle::State_None)
        debug << ", state=" << option->state;
    if (option->styleObject && !option->styleObject->isWidgetType())
        debug << ", styleObject=" << QtDiag::formatQObject(option->styleObject);
    debug << ')';
    return debug;
}

namespace QtDiag {

DebugProxyStyle::DebugProxyStyle(QStyle *style) : QProxyStyle(style)
{
    const qreal devicePixelRatio = qApp->devicePixelRatio();
    qDebug() << __FUNCTION__ << QT_VERSION_STR
        << QGuiApplication::platformName()
        << style->objectName() << "devicePixelRatio=" << devicePixelRatio;
}

void DebugProxyStyle::drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    qDebug() << __FUNCTION__ << "element=" << element << option << QtDiag::formatQObject(widget);
    QProxyStyle::drawPrimitive( element, option, painter, widget);
}

void DebugProxyStyle::drawControl(QStyle::ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    qDebug() << __FUNCTION__ << "element=" << element << option << QtDiag::formatQObject(widget);
    QProxyStyle::drawControl(element, option, painter, widget);
}

void DebugProxyStyle::drawComplexControl(QStyle::ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const
{
    qDebug() << __FUNCTION__ << "control=" << control << option << QtDiag::formatQObject(widget);
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
    qDebug() << __FUNCTION__ << size << "type=" << type << option
        << QtDiag::formatQObject(widget) << "returns" << result;
    return result;
}

QRect DebugProxyStyle::subElementRect(QStyle::SubElement element, const QStyleOption *option, const QWidget *widget) const
{
    const QRect result = QProxyStyle::subElementRect(element, option, widget);
    qDebug() << __FUNCTION__ << "element=" << element << option
        << QtDiag::formatQObject(widget) << "returns" << result;
    return result;
}

QRect DebugProxyStyle::subControlRect(QStyle::ComplexControl cc, const QStyleOptionComplex *opt, QStyle::SubControl sc, const QWidget *widget) const
{
    const QRect result = QProxyStyle::subControlRect(cc, opt, sc, widget);
    qDebug() << __FUNCTION__ << "cc=" << cc << "sc=" << sc << opt
        << QtDiag::formatQObject(widget) << "returns" << result;
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

int DebugProxyStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                               QStyleHintReturn *returnData) const
{
    const int result = QProxyStyle::styleHint(hint, option, widget, returnData);
    qDebug() << __FUNCTION__ << hint << option << QtDiag::formatQObject(widget) << "returnData="
        << returnData << "returns" << result;
    return result;
}

int DebugProxyStyle::pixelMetric(QStyle::PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    const int result = QProxyStyle::pixelMetric(metric, option, widget);
    qDebug() << __FUNCTION__ << "metric=" << metric << option
        << QtDiag::formatQObject(widget) << "returns" << result;
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
