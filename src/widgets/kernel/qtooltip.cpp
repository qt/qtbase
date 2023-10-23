// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtWidgets/private/qtwidgetsglobal_p.h>

#include <qapplication.h>
#include <qevent.h>
#include <qpointer.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <qstylepainter.h>
#include <qtimer.h>
#if QT_CONFIG(effects)
#include <private/qeffects_p.h>
#endif
#include <qtextdocument.h>
#include <qdebug.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qplatformcursor.h>
#include <private/qstylesheetstyle_p.h>

#include <qlabel.h>
#include <QtWidgets/private/qlabel_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <qtooltip.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QToolTip

    \brief The QToolTip class provides tool tips (balloon help) for any
    widget.

    \ingroup helpsystem
    \inmodule QtWidgets

    The tip is a short piece of text reminding the user of the
    widget's function. It is drawn immediately below the given
    position in a distinctive black-on-yellow color combination. The
    tip can be any \l{QTextEdit}{rich text} formatted string.

    Rich text displayed in a tool tip is implicitly word-wrapped unless
    specified differently with \c{<p style='white-space:pre'>}.

    UI elements that are created via \l{QAction} use the tooltip property
    of the QAction, so for most interactive UI elements, setting that
    property is the easiest way to provide tool tips.

    \snippet tooltips/main.cpp action_tooltip

    For any other widgets, the simplest and most common way to set
    a widget's tool tip is by calling its QWidget::setToolTip() function.

    \snippet tooltips/main.cpp static_tooltip

    It is also possible to show different tool tips for different
    regions of a widget, by using a QHelpEvent of type
    QEvent::ToolTip. Intercept the help event in your widget's \l
    {QWidget::}{event()} function and call QToolTip::showText() with
    the text you want to display.

    \snippet tooltips/main.cpp dynamic_tooltip

    If you are calling QToolTip::hideText(), or QToolTip::showText()
    with an empty string, as a result of a \l{QEvent::}{ToolTip}-event you
    should also call \l{QEvent::}{ignore()} on the event, to signal
    that you don't want to start any tooltip specific modes.

    Note that, if you want to show tooltips in an item view, the
    model/view architecture provides functionality to set an item's
    tool tip; e.g., the QTableWidgetItem::setToolTip() function.
    However, if you want to provide custom tool tips in an item view,
    you must intercept the help event in the
    QAbstractItemView::viewportEvent() function and handle it yourself.

    The default tool tip color and font can be customized with
    setPalette() and setFont(). When a tooltip is currently on
    display, isVisible() returns \c true and text() the currently visible
    text.

    \note Tool tips use the inactive color group of QPalette, because tool
    tips are not active windows.

    \sa QWidget::toolTip, QAction::toolTip
*/

class QTipLabel : public QLabel
{
    Q_OBJECT
public:
    QTipLabel(const QString &text, const QPoint &pos, QWidget *w, int msecDisplayTime);
    ~QTipLabel();
    static QTipLabel *instance;

    void adjustTooltipScreen(const QPoint &pos);
    void updateSize(const QPoint &pos);

    bool eventFilter(QObject *, QEvent *) override;

    QBasicTimer hideTimer, expireTimer;

    bool fadingOut;

    void reuseTip(const QString &text, int msecDisplayTime, const QPoint &pos);
    void hideTip();
    void hideTipImmediately();
    void setTipRect(QWidget *w, const QRect &r);
    void restartExpireTimer(int msecDisplayTime);
    bool tipChanged(const QPoint &pos, const QString &text, QObject *o);
    void placeTip(const QPoint &pos, QWidget *w);

    static QScreen *getTipScreen(const QPoint &pos, QWidget *w);
protected:
    void timerEvent(QTimerEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

#ifndef QT_NO_STYLE_STYLESHEET
public slots:
    /** \internal
      Cleanup the _q_stylesheet_parent property.
     */
    void styleSheetParentDestroyed() {
        setProperty("_q_stylesheet_parent", QVariant());
        styleSheetParent = nullptr;
    }

private:
    QWidget *styleSheetParent;
#endif

private:
    QWidget *widget;
    QRect rect;
};

QTipLabel *QTipLabel::instance = nullptr;

QTipLabel::QTipLabel(const QString &text, const QPoint &pos, QWidget *w, int msecDisplayTime)
    : QLabel(w, Qt::ToolTip | Qt::BypassGraphicsProxyWidget)
#ifndef QT_NO_STYLE_STYLESHEET
    , styleSheetParent(nullptr)
#endif
    , widget(nullptr)
{
    delete instance;
    instance = this;
    setForegroundRole(QPalette::ToolTipText);
    setBackgroundRole(QPalette::ToolTipBase);
    setPalette(QToolTip::palette());
    ensurePolished();
    setMargin(1 + style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, nullptr, this));
    setFrameStyle(QFrame::NoFrame);
    setAlignment(Qt::AlignLeft);
    setIndent(1);
    qApp->installEventFilter(this);
    setWindowOpacity(style()->styleHint(QStyle::SH_ToolTipLabel_Opacity, nullptr, this) / 255.0);
    setMouseTracking(true);
    fadingOut = false;
    reuseTip(text, msecDisplayTime, pos);
}

void QTipLabel::restartExpireTimer(int msecDisplayTime)
{
    Q_D(const QLabel);
    const qsizetype textLength = d->needTextControl() ? d->control->toPlainText().size() : text().size();
    qsizetype time = 10000 + 40 * qMax(0, textLength - 100);
    if (msecDisplayTime > 0)
        time = msecDisplayTime;
    expireTimer.start(time, this);
    hideTimer.stop();
}

void QTipLabel::reuseTip(const QString &text, int msecDisplayTime, const QPoint &pos)
{
#ifndef QT_NO_STYLE_STYLESHEET
    if (styleSheetParent){
        disconnect(styleSheetParent, SIGNAL(destroyed()),
                   QTipLabel::instance, SLOT(styleSheetParentDestroyed()));
        styleSheetParent = nullptr;
    }
#endif

    setText(text);
    updateSize(pos);
    restartExpireTimer(msecDisplayTime);
}

void  QTipLabel::updateSize(const QPoint &pos)
{
    d_func()->setScreenForPoint(pos);
    // Ensure that we get correct sizeHints by placing this window on the right screen.
    QFontMetrics fm(font());
    QSize extra(1, 0);
    // Make it look good with the default ToolTip font on Mac, which has a small descent.
    if (fm.descent() == 2 && fm.ascent() >= 11)
        ++extra.rheight();
    setWordWrap(Qt::mightBeRichText(text()));
    QSize sh = sizeHint();
    const QScreen *screen = getTipScreen(pos, this);
    if (!wordWrap() && sh.width() > screen->geometry().width()) {
        setWordWrap(true);
        sh = sizeHint();
    }
    resize(sh + extra);
}

void QTipLabel::paintEvent(QPaintEvent *ev)
{
    QStylePainter p(this);
    QStyleOptionFrame opt;
    opt.initFrom(this);
    p.drawPrimitive(QStyle::PE_PanelTipLabel, opt);
    p.end();

    QLabel::paintEvent(ev);
}

void QTipLabel::resizeEvent(QResizeEvent *e)
{
    QStyleHintReturnMask frameMask;
    QStyleOption option;
    option.initFrom(this);
    if (style()->styleHint(QStyle::SH_ToolTip_Mask, &option, this, &frameMask))
        setMask(frameMask.region);

    QLabel::resizeEvent(e);
}

void QTipLabel::mouseMoveEvent(QMouseEvent *e)
{
    if (!rect.isNull()) {
        QPoint pos = e->globalPosition().toPoint();
        if (widget)
            pos = widget->mapFromGlobal(pos);
        if (!rect.contains(pos))
            hideTip();
    }
    QLabel::mouseMoveEvent(e);
}

QTipLabel::~QTipLabel()
{
    instance = nullptr;
}

void QTipLabel::hideTip()
{
    if (!hideTimer.isActive())
        hideTimer.start(300, this);
}

void QTipLabel::hideTipImmediately()
{
    close(); // to trigger QEvent::Close which stops the animation
    deleteLater();
}

void QTipLabel::setTipRect(QWidget *w, const QRect &r)
{
    if (Q_UNLIKELY(!r.isNull() && !w)) {
        qWarning("QToolTip::setTipRect: Cannot pass null widget if rect is set");
        return;
    }
    widget = w;
    rect = r;
}

void QTipLabel::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == hideTimer.timerId()
        || e->timerId() == expireTimer.timerId()){
        hideTimer.stop();
        expireTimer.stop();
        hideTipImmediately();
    }
}

bool QTipLabel::eventFilter(QObject *o, QEvent *e)
{
    switch (e->type()) {
#ifdef Q_OS_MACOS
    case QEvent::KeyPress:
    case QEvent::KeyRelease: {
        const int key = static_cast<QKeyEvent *>(e)->key();
        // Anything except key modifiers or caps-lock, etc.
        if (key < Qt::Key_Shift || key > Qt::Key_ScrollLock)
            hideTipImmediately();
        break;
    }
#endif
    case QEvent::Leave:
        hideTip();
        break;


#if defined (Q_OS_QNX)  || defined (Q_OS_WASM) // On QNX the window activate and focus events are delayed and will appear
                       // after the window is shown.
    case QEvent::WindowActivate:
    case QEvent::FocusIn:
        return false;
    case QEvent::WindowDeactivate:
        if (o != this)
            return false;
        hideTipImmediately();
        break;
    case QEvent::FocusOut:
        if (reinterpret_cast<QWindow*>(o) != windowHandle())
            return false;
        hideTipImmediately();
        break;
#else
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
    case QEvent::FocusIn:
    case QEvent::FocusOut:
#endif
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel:
        hideTipImmediately();
        break;

    case QEvent::MouseMove:
        if (o == widget && !rect.isNull() && !rect.contains(static_cast<QMouseEvent*>(e)->position().toPoint()))
            hideTip();
    default:
        break;
    }
    return false;
}

QScreen *QTipLabel::getTipScreen(const QPoint &pos, QWidget *w)
{
    QScreen *guess = w ? w->screen() : QGuiApplication::primaryScreen();
    QScreen *exact = guess->virtualSiblingAt(pos);
    return exact ? exact : guess;
}

void QTipLabel::placeTip(const QPoint &pos, QWidget *w)
{
#ifndef QT_NO_STYLE_STYLESHEET
    if (testAttribute(Qt::WA_StyleSheet) || (w && qt_styleSheet(w->style()))) {
        //the stylesheet need to know the real parent
        QTipLabel::instance->setProperty("_q_stylesheet_parent", QVariant::fromValue(w));
        //we force the style to be the QStyleSheetStyle, and force to clear the cache as well.
        QTipLabel::instance->setStyleSheet("/* */"_L1);

        // Set up for cleaning up this later...
        QTipLabel::instance->styleSheetParent = w;
        if (w) {
            connect(w, SIGNAL(destroyed()),
                QTipLabel::instance, SLOT(styleSheetParentDestroyed()));
            // QTBUG-64550: A font inherited by the style sheet might change the size,
            // particular on Windows, where the tip is not parented on a window.
            QTipLabel::instance->updateSize(pos);
        }
    }
#endif //QT_NO_STYLE_STYLESHEET

    QPoint p = pos;
    const QScreen *screen = getTipScreen(pos, w);
    // a QScreen's handle *should* never be null, so this is a bit paranoid
    if (const QPlatformScreen *platformScreen = screen ? screen->handle() : nullptr) {
        QPlatformCursor *cursor = platformScreen->cursor();
        // default implementation of QPlatformCursor::size() returns QSize(16, 16)
        const QSize nativeSize = cursor ? cursor->size() : QSize(16, 16);
        const QSize cursorSize = QHighDpi::fromNativePixels(nativeSize,
                                                            platformScreen);
        QPoint offset(2, cursorSize.height());
        // assuming an arrow shape, we can just move to the side for very large cursors
        if (cursorSize.height() > 2 * this->height())
            offset = QPoint(cursorSize.width() / 2, 0);

        p += offset;

        QRect screenRect = screen->geometry();
        if (p.x() + this->width() > screenRect.x() + screenRect.width())
        p.rx() -= 4 + this->width();
        if (p.y() + this->height() > screenRect.y() + screenRect.height())
        p.ry() -= 24 + this->height();
        if (p.y() < screenRect.y())
            p.setY(screenRect.y());
        if (p.x() + this->width() > screenRect.x() + screenRect.width())
            p.setX(screenRect.x() + screenRect.width() - this->width());
        if (p.x() < screenRect.x())
            p.setX(screenRect.x());
        if (p.y() + this->height() > screenRect.y() + screenRect.height())
            p.setY(screenRect.y() + screenRect.height() - this->height());
    }
    this->move(p);
}

bool QTipLabel::tipChanged(const QPoint &pos, const QString &text, QObject *o)
{
    if (QTipLabel::instance->text() != text)
        return true;

    if (o != widget)
        return true;

    if (!rect.isNull())
        return !rect.contains(pos);
    else
       return false;
}

/*!
    Shows \a text as a tool tip, with the global position \a pos as
    the point of interest. The tool tip will be shown with a platform
    specific offset from this point of interest.

    If you specify a non-empty rect the tip will be hidden as soon
    as you move your cursor out of this area.

    The \a rect is in the coordinates of the widget you specify with
    \a w. If the \a rect is not empty you must specify a widget.
    Otherwise this argument can be \nullptr but it is used to
    determine the appropriate screen on multi-head systems.

    The \a msecDisplayTime parameter specifies for how long the tool tip
    will be displayed, in milliseconds. With the default value of -1, the
    time is based on the length of the text.

    If \a text is empty the tool tip is hidden. If the text is the
    same as the currently shown tooltip, the tip will \e not move.
    You can force moving by first hiding the tip with an empty text,
    and then showing the new tip at the new position.
*/

void QToolTip::showText(const QPoint &pos, const QString &text, QWidget *w, const QRect &rect, int msecDisplayTime)
{
    if (QTipLabel::instance && QTipLabel::instance->isVisible()) { // a tip does already exist
        if (text.isEmpty()){ // empty text means hide current tip
            QTipLabel::instance->hideTip();
            return;
        } else if (!QTipLabel::instance->fadingOut) {
            // If the tip has changed, reuse the one
            // that is showing (removes flickering)
            QPoint localPos = pos;
            if (w)
                localPos = w->mapFromGlobal(pos);
            if (QTipLabel::instance->tipChanged(localPos, text, w)){
                QTipLabel::instance->reuseTip(text, msecDisplayTime, pos);
                QTipLabel::instance->setTipRect(w, rect);
                QTipLabel::instance->placeTip(pos, w);
            }
            return;
        }
    }

    if (!text.isEmpty()) { // no tip can be reused, create new tip:
        QWidget *tipLabelParent = [w]() -> QWidget* {
#ifdef Q_OS_WIN32
            // On windows, we can't use the widget as parent otherwise the window will be
            // raised when the tooltip will be shown
            Q_UNUSED(w);
            return nullptr;
#else
            return w;
#endif
        }();
        new QTipLabel(text, pos, tipLabelParent, msecDisplayTime); // sets QTipLabel::instance to itself
        QWidgetPrivate::get(QTipLabel::instance)->setScreen(QTipLabel::getTipScreen(pos, w));
        QTipLabel::instance->setTipRect(w, rect);
        QTipLabel::instance->placeTip(pos, w);
        QTipLabel::instance->setObjectName("qtooltip_label"_L1);

#if QT_CONFIG(effects)
        if (QApplication::isEffectEnabled(Qt::UI_FadeTooltip))
            qFadeEffect(QTipLabel::instance);
        else if (QApplication::isEffectEnabled(Qt::UI_AnimateTooltip))
            qScrollEffect(QTipLabel::instance);
        else
            QTipLabel::instance->showNormal();
#else
        QTipLabel::instance->showNormal();
#endif
    }
}

/*!
    \fn void QToolTip::hideText()
    \since 4.2

    Hides the tool tip. This is the same as calling showText() with an
    empty string.

    \sa showText()
*/


/*!
  \since 4.4

  Returns \c true if a tooltip is currently shown.

  \sa showText()
 */
bool QToolTip::isVisible()
{
    return (QTipLabel::instance != nullptr && QTipLabel::instance->isVisible());
}

/*!
  \since 4.4

  Returns the tooltip text, if a tooltip is visible, or an
  empty string if a tooltip is not visible.
 */
QString QToolTip::text()
{
    if (QTipLabel::instance)
        return QTipLabel::instance->text();
    return QString();
}


Q_GLOBAL_STATIC(QPalette, tooltip_palette)

/*!
    Returns the palette used to render tooltips.

    \note Tool tips use the inactive color group of QPalette, because tool
    tips are not active windows.
*/
QPalette QToolTip::palette()
{
    return *tooltip_palette();
}

/*!
    \since 4.2

    Returns the font used to render tooltips.
*/
QFont QToolTip::font()
{
    return QApplication::font("QTipLabel");
}

/*!
    \since 4.2

    Sets the \a palette used to render tooltips.

    \note Tool tips use the inactive color group of QPalette, because tool
    tips are not active windows.
*/
void QToolTip::setPalette(const QPalette &palette)
{
    *tooltip_palette() = palette;
    if (QTipLabel::instance)
        QTipLabel::instance->setPalette(palette);
}

/*!
    \since 4.2

    Sets the \a font used to render tooltips.
*/
void QToolTip::setFont(const QFont &font)
{
    QApplication::setFont(font, "QTipLabel");
}

QT_END_NAMESPACE

#include "qtooltip.moc"
