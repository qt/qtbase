// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qstyle.h>
#include <private/qproxystyle_p.h>
#include <private/qapplication_p.h>
#include "qproxystyle.h"
#include "qstylefactory.h"
#include <private/qstyle_p.h>

#if !defined(QT_NO_STYLE_PROXY) || defined(QT_PLUGIN)

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QProxyStyle

    \brief The QProxyStyle class is a convenience class that simplifies
    dynamically overriding QStyle elements.

    \since 4.6

    \inmodule QtWidgets

    A QProxyStyle wraps a QStyle (usually the default system style) for the
    purpose of dynamically overriding painting or other specific style behavior.

    The following example shows how to override the shortcut underline
    behavior on any platform:

    \snippet code/src_gui_qproxystyle.cpp 1

    Warning: The \l {QCommonStyle} {common styles} provided by Qt will
    respect this hint, because they call QStyle::proxy(), but there is
    no guarantee that QStyle::proxy() will be called for user defined
    or system controlled styles. It would not work on a Mac, for
    example, where menus are handled by the operating system.

    When a proxy style should be set on a specific widget only, you have
    to make sure to not set the proxy on the global application style which
    is returned by QWidget::style(). You have to create a separate custom style
    for the widget similar to:

    \snippet code/src_gui_qproxystyle.cpp 2

    \sa QStyle
*/

void QProxyStylePrivate::ensureBaseStyle() const
{
    Q_Q(const QProxyStyle);

    if (baseStyle)
        return;

    if (!baseStyle && !QApplicationPrivate::styleOverride.isEmpty()) {
        baseStyle = QStyleFactory::create(QApplicationPrivate::styleOverride);
        if (baseStyle) {
            // If baseStyle is an instance of the same proxyStyle
            // we destroy it and fall back to the desktop style
            if (qstrcmp(baseStyle->metaObject()->className(),
                        q->metaObject()->className()) == 0) {
                delete baseStyle;
                baseStyle = nullptr;
            }
        }
    }

    if (!baseStyle) // Use application desktop style
        baseStyle = QStyleFactory::create(QApplicationPrivate::desktopStyleKey());

    if (!baseStyle) // Fallback to windows style
        baseStyle = QStyleFactory::create("windows"_L1);

    baseStyle->setProxy(const_cast<QProxyStyle*>(q));
    baseStyle->setParent(const_cast<QProxyStyle*>(q)); // Take ownership
}

/*!
  Constructs a QProxyStyle object for overriding behavior in the
  specified \a style, or in the default native \l{QApplication::style()}
  {style} if \a style is not specified.

  Ownership of \a style is transferred to QProxyStyle.
*/
QProxyStyle::QProxyStyle(QStyle *style) :
    QCommonStyle(*new QProxyStylePrivate())
{
    Q_D(QProxyStyle);
    if (style) {
        d->baseStyle = style;
        style->setProxy(this);
        style->setParent(this); // Take ownership
    }
}

/*!
    Constructs a QProxyStyle object for overriding behavior in
    the base style specified by style \a key, or in the current
    \l{QApplication::style()}{application style} if the specified
    style \a key is unrecognized.

    \sa QStyleFactory::create()
*/
QProxyStyle::QProxyStyle(const QString &key)
    : QProxyStyle(QStyleFactory::create(key))
{
}

/*!
    Destroys the QProxyStyle object.
*/
QProxyStyle::~QProxyStyle()
{
}

/*!
    Returns the proxy base style object. If no base style
    is set on the proxy style, QProxyStyle will create
    an instance of the application style instead.

    \sa setBaseStyle(), QStyle
*/
QStyle *QProxyStyle::baseStyle() const
{
    Q_D (const QProxyStyle);
    d->ensureBaseStyle();
    return d->baseStyle;
}

/*!
    Sets the base style that should be proxied.

    Ownership of \a style is transferred to QProxyStyle.

    If style is \nullptr, a desktop-dependent style will be
    assigned automatically.
*/
void QProxyStyle::setBaseStyle(QStyle *style)
{
    Q_D (QProxyStyle);

    if (d->baseStyle && d->baseStyle->parent() == this)
        d->baseStyle->deleteLater();

    d->baseStyle = style;

    if (d->baseStyle) {
        d->baseStyle->setProxy(this);
        d->baseStyle->setParent(this);
    }
}

/*! \reimp
 */
void QProxyStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    Q_D (const QProxyStyle);
    d->ensureBaseStyle();
    d->baseStyle->drawPrimitive(element, option, painter, widget);
}

/*!
  \reimp
 */
void QProxyStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    Q_D (const QProxyStyle);
    d->ensureBaseStyle();
    d->baseStyle->drawControl(element, option, painter, widget);
}

/*! \reimp
 */
void QProxyStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const
{
    Q_D (const QProxyStyle);
    d->ensureBaseStyle();
    d->baseStyle->drawComplexControl(control, option, painter, widget);
}

/*! \reimp
 */
void QProxyStyle::drawItemText(QPainter *painter, const QRect &rect, int flags, const QPalette &pal, bool enabled,
                               const QString &text, QPalette::ColorRole textRole) const
{
    Q_D (const QProxyStyle);
    d->ensureBaseStyle();
    d->baseStyle->drawItemText(painter, rect, flags, pal, enabled, text, textRole);
}

/*! \reimp
 */
void QProxyStyle::drawItemPixmap(QPainter *painter, const QRect &rect, int alignment, const QPixmap &pixmap) const
{
    Q_D (const QProxyStyle);
    d->ensureBaseStyle();
    d->baseStyle->drawItemPixmap(painter, rect, alignment, pixmap);
}

/*! \reimp
 */
QSize QProxyStyle::sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget) const
{
    Q_D (const QProxyStyle);
    d->ensureBaseStyle();
    return d->baseStyle->sizeFromContents(type, option, size, widget);
}

/*! \reimp
 */
QRect QProxyStyle::subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const
{
    Q_D (const QProxyStyle);
    d->ensureBaseStyle();
    return d->baseStyle->subElementRect(element, option, widget);
}

/*! \reimp
 */
QRect QProxyStyle::subControlRect(ComplexControl cc, const QStyleOptionComplex *option, SubControl sc, const QWidget *widget) const
{
    Q_D (const QProxyStyle);
    d->ensureBaseStyle();
    return d->baseStyle->subControlRect(cc, option, sc, widget);
}

/*! \reimp
 */
QRect QProxyStyle::itemTextRect(const QFontMetrics &fm, const QRect &r, int flags, bool enabled, const QString &text) const
{
    Q_D (const QProxyStyle);
    d->ensureBaseStyle();
    return d->baseStyle->itemTextRect(fm, r, flags, enabled, text);
}

/*! \reimp
 */
QRect QProxyStyle::itemPixmapRect(const QRect &r, int flags, const QPixmap &pixmap) const
{
    Q_D (const QProxyStyle);
    d->ensureBaseStyle();
    return d->baseStyle->itemPixmapRect(r, flags, pixmap);
}

/*! \reimp
 */
QStyle::SubControl QProxyStyle::hitTestComplexControl(ComplexControl control, const QStyleOptionComplex *option, const QPoint &pos, const QWidget *widget) const
{
    Q_D (const QProxyStyle);
    d->ensureBaseStyle();
    return d->baseStyle->hitTestComplexControl(control, option, pos, widget);
}

/*! \reimp
 */
int QProxyStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *returnData) const
{
    Q_D (const QProxyStyle);
    d->ensureBaseStyle();
    return d->baseStyle->styleHint(hint, option, widget, returnData);
}

/*! \reimp
 */
int QProxyStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    Q_D (const QProxyStyle);
    d->ensureBaseStyle();
    return d->baseStyle->pixelMetric(metric, option, widget);
}

/*! \reimp
 */
QPixmap QProxyStyle::standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt, const QWidget *widget) const
{
    Q_D (const QProxyStyle);
    d->ensureBaseStyle();
    return d->baseStyle->standardPixmap(standardPixmap, opt, widget);
}

/*! \reimp
 */
QPixmap QProxyStyle::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap, const QStyleOption *opt) const
{
    Q_D (const QProxyStyle);
    d->ensureBaseStyle();
    return d->baseStyle->generatedIconPixmap(iconMode, pixmap, opt);
}

/*! \reimp
 */
QPalette QProxyStyle::standardPalette() const
{
    Q_D (const QProxyStyle);
    d->ensureBaseStyle();
    return d->baseStyle->standardPalette();
}

/*! \reimp
 */
void QProxyStyle::polish(QWidget *widget)
{
    Q_D (QProxyStyle);
    d->ensureBaseStyle();
    d->baseStyle->polish(widget);
}

/*! \reimp
 */
void QProxyStyle::polish(QPalette &pal)
{
    Q_D (QProxyStyle);
    d->ensureBaseStyle();
    d->baseStyle->polish(pal);
}

/*! \reimp
 */
void QProxyStyle::polish(QApplication *app)
{
    Q_D (QProxyStyle);
    d->ensureBaseStyle();
    d->baseStyle->polish(app);
}

/*! \reimp
 */
void QProxyStyle::unpolish(QWidget *widget)
{
    Q_D (QProxyStyle);
    d->ensureBaseStyle();
    d->baseStyle->unpolish(widget);
}

/*! \reimp
 */
void QProxyStyle::unpolish(QApplication *app)
{
    Q_D (QProxyStyle);
    d->ensureBaseStyle();
    d->baseStyle->unpolish(app);
}

/*! \reimp
 */
bool QProxyStyle::event(QEvent *e)
{
    Q_D (QProxyStyle);

    switch (e->type()) {
    // The Mac style relies on these events in order to set the focus frame
    case QEvent::FocusIn:
    case QEvent::FocusOut:
        d->ensureBaseStyle();
        return QApplication::sendEvent(d->baseStyle, e);
    default:
        break;
    }

    return QCommonStyle::event(e);
}

/*!
  Returns an icon for the given \a standardIcon.

  Reimplement this slot to provide your own icons in a QStyle
  subclass. The \a option argument can be used to pass extra
  information required to find the appropriate icon. The \a widget
  argument is optional and can also be used to help find the icon.
 */
QIcon QProxyStyle::standardIcon(StandardPixmap standardIcon,
                                const QStyleOption *option,
                                const QWidget *widget) const
{
    Q_D (const QProxyStyle);
    d->ensureBaseStyle();
    return d->baseStyle->standardIcon(standardIcon, option, widget);
}

/*!
  This slot is called by layoutSpacing() to determine the spacing that
  should be used between \a control1 and \a control2 in a layout. \a
  orientation specifies whether the controls are laid out side by side
  or stacked vertically. The \a option parameter can be used to pass
  extra information about the parent widget.  The \a widget parameter
  is optional and can also be used if \a option is \nullptr.

  The default implementation returns -1.

  \sa combinedLayoutSpacing()
 */
int QProxyStyle::layoutSpacing(QSizePolicy::ControlType control1,
                               QSizePolicy::ControlType control2,
                               Qt::Orientation orientation,
                               const QStyleOption *option,
                               const QWidget *widget) const
{
    Q_D (const QProxyStyle);
    d->ensureBaseStyle();
    return d->baseStyle->layoutSpacing(control1, control2, orientation, option, widget);
}

QT_END_NAMESPACE

#include "moc_qproxystyle.cpp"

#endif // QT_NO_STYLE_PROXY
