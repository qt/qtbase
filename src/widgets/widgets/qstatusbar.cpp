// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qstatusbar.h"

#include "qlist.h"
#include "qdebug.h"
#include "qevent.h"
#include "qlayout.h"
#include "qpainter.h"
#include "qtimer.h"
#include "qstyle.h"
#include "qstyleoption.h"
#if QT_CONFIG(sizegrip)
#include "qsizegrip.h"
#endif
#if QT_CONFIG(mainwindow)
#include "qmainwindow.h"
#endif

#if QT_CONFIG(accessibility)
#include "qaccessible.h"
#endif

#include <private/qlayoutengine_p.h>
#include <private/qwidget_p.h>

QT_BEGIN_NAMESPACE

class QStatusBarPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QStatusBar)
public:
    QStatusBarPrivate() {}

    enum ItemCategory
    {
        Normal,
        Permanent
    };

    struct SBItem {
        QWidget *widget = nullptr;
        int stretch = 0;
        ItemCategory category = Normal;
        bool isPermanent() const { return category == Permanent; }
    };

    QList<SBItem> items;
    QString tempItem;

    QBoxLayout *box;
    QTimer *timer;

#if QT_CONFIG(sizegrip)
    QSizeGrip *resizer;
    bool showSizeGrip;
#endif

    int savedStrut;

    int indexToLastNonPermanentWidget() const
    {
        int i = items.size() - 1;
        for (; i >= 0; --i) {
            const SBItem &item = items.at(i);
            if (!item.isPermanent())
                break;
        }
        return i;
    }

#if QT_CONFIG(sizegrip)
    void tryToShowSizeGrip()
    {
        if (!showSizeGrip)
            return;
        showSizeGrip = false;
        if (!resizer || resizer->isVisible())
            return;
        resizer->setAttribute(Qt::WA_WState_ExplicitShowHide, false);
        QMetaObject::invokeMethod(resizer, "_q_showIfNotHidden", Qt::DirectConnection);
        resizer->setAttribute(Qt::WA_WState_ExplicitShowHide, false);
    }
#endif

    QRect messageRect() const;
};


QRect QStatusBarPrivate::messageRect() const
{
    Q_Q(const QStatusBar);
    const bool rtl = q->layoutDirection() == Qt::RightToLeft;

    int left = 6;
    int right = q->width() - 12;

#if QT_CONFIG(sizegrip)
    if (resizer && resizer->isVisible()) {
        if (rtl)
            left = resizer->x() + resizer->width();
        else
            right = resizer->x();
    }
#endif

    for (const auto &item : items) {
        if (item.isPermanent() && item.widget->isVisible()) {
            if (rtl)
                left = qMax(left, item.widget->x() + item.widget->width() + 2);
            else
                right = qMin(right, item.widget->x() - 2);
            break;
        }
    }
    return QRect(left, 0, right-left, q->height());
}


/*!
    \class QStatusBar
    \brief The QStatusBar class provides a horizontal bar suitable for
    presenting status information.

    \ingroup mainwindow-classes
    \ingroup helpsystem
    \inmodule QtWidgets

    Each status indicator falls into one of three categories:

    \list
    \li \e Temporary - briefly occupies most of the status bar. Used
        to explain tool tip texts or menu entries, for example.
    \li \e Normal - occupies part of the status bar and may be hidden
        by temporary messages. Used to display the page and line
        number in a word processor, for example.
    \li \e Permanent - is never hidden. Used for important mode
        indications, for example, some applications put a Caps Lock
        indicator in the status bar.
    \endlist

    QStatusBar lets you display all three types of indicators.

    Typically, a request for the status bar functionality occurs in
    relation to a QMainWindow object. QMainWindow provides a main
    application window, with a menu bar, tool bars, dock widgets \e
    and a status bar around a large central widget. The status bar can
    be retrieved using the QMainWindow::statusBar() function, and
    replaced using the QMainWindow::setStatusBar() function.

    Use the showMessage() slot to display a \e temporary message:

    \snippet code/src_gui_widgets_qstatusbar.cpp 1

    To remove a temporary message, use the clearMessage() slot, or set
    a time limit when calling showMessage(). For example:

    \snippet code/src_gui_widgets_qstatusbar.cpp 2

    Use the currentMessage() function to retrieve the temporary
    message currently shown. The QStatusBar class also provide the
    messageChanged() signal which is emitted whenever the temporary
    status message changes.

    \target permanent message
    \e Normal and \e Permanent messages are displayed by creating a
    small widget (QLabel, QProgressBar or even QToolButton) and then
    adding it to the status bar using the addWidget() or the
    addPermanentWidget() function. Use the removeWidget() function to
    remove such messages from the status bar.

    \snippet code/src_gui_widgets_qstatusbar.cpp 0

    By default QStatusBar provides a QSizeGrip in the lower-right
    corner. You can disable it using the setSizeGripEnabled()
    function. Use the isSizeGripEnabled() function to determine the
    current status of the size grip.

    \image fusion-statusbar-sizegrip.png A status bar shown in the Fusion widget style

    \sa QMainWindow, QStatusTipEvent
*/


/*!
    Constructs a status bar with a size grip and the given \a parent.

    \sa setSizeGripEnabled()
*/
QStatusBar::QStatusBar(QWidget * parent)
    : QWidget(*new QStatusBarPrivate, parent, { })
{
    Q_D(QStatusBar);
    d->box = nullptr;
    d->timer = nullptr;

#if QT_CONFIG(sizegrip)
    d->resizer = nullptr;
    setSizeGripEnabled(true); // causes reformat()
#else
    reformat();
#endif
}

/*!
    Destroys this status bar and frees any allocated resources and
    child widgets.
*/
QStatusBar::~QStatusBar()
{
}


/*!
    Adds the given \a widget to this status bar, reparenting the
    widget if it isn't already a child of this QStatusBar object. The
    \a stretch parameter is used to compute a suitable size for the
    given \a widget as the status bar grows and shrinks. The default
    stretch factor is 0, i.e giving the widget a minimum of space.

    The widget is located to the far left of the first permanent
    widget (see addPermanentWidget()) and may be obscured by temporary
    messages.

    \sa insertWidget(), removeWidget(), addPermanentWidget()
*/

void QStatusBar::addWidget(QWidget * widget, int stretch)
{
    if (!widget)
        return;
    insertWidget(d_func()->indexToLastNonPermanentWidget() + 1, widget, stretch);
}

/*!
    \since 4.2

    Inserts the given \a widget at the given \a index to this status bar,
    reparenting the widget if it isn't already a child of this
    QStatusBar object. If \a index is out of range, the widget is appended
    (in which case it is the actual index of the widget that is returned).

    The \a stretch parameter is used to compute a suitable size for
    the given \a widget as the status bar grows and shrinks. The
    default stretch factor is 0, i.e giving the widget a minimum of
    space.

    The widget is located to the far left of the first permanent
    widget (see addPermanentWidget()) and may be obscured by temporary
    messages.

    \sa addWidget(), removeWidget(), addPermanentWidget()
*/
int QStatusBar::insertWidget(int index, QWidget *widget, int stretch)
{
    if (!widget)
        return -1;

    Q_D(QStatusBar);
    QStatusBarPrivate::SBItem item{widget, stretch, QStatusBarPrivate::Normal};

    int idx = d->indexToLastNonPermanentWidget();
    if (Q_UNLIKELY(index < 0 || index > d->items.size() || (idx >= 0 && index > idx + 1))) {
        qWarning("QStatusBar::insertWidget: Index out of range (%d), appending widget", index);
        index = idx + 1;
    }
    d->items.insert(index, item);

    if (!d->tempItem.isEmpty())
        widget->hide();

    reformat();
    if (!widget->isHidden() || !widget->testAttribute(Qt::WA_WState_ExplicitShowHide))
        widget->show();

    return index;
}

/*!
    Adds the given \a widget permanently to this status bar,
    reparenting the widget if it isn't already a child of this
    QStatusBar object. The \a stretch parameter is used to compute a
    suitable size for the given \a widget as the status bar grows and
    shrinks. The default stretch factor is 0, i.e giving the widget a
    minimum of space.

    Permanently means that the widget may not be obscured by temporary
    messages. It is located at the far right of the status bar.

    \sa insertPermanentWidget(), removeWidget(), addWidget()
*/

void QStatusBar::addPermanentWidget(QWidget * widget, int stretch)
{
    if (!widget)
        return;
    insertPermanentWidget(d_func()->items.size(), widget, stretch);
}

/*!
    \since 4.2

    Inserts the given \a widget at the given \a index permanently to this status bar,
    reparenting the widget if it isn't already a child of this
    QStatusBar object. If \a index is out of range, the widget is appended
    (in which case it is the actual index of the widget that is returned).

    The \a stretch parameter is used to compute a
    suitable size for the given \a widget as the status bar grows and
    shrinks. The default stretch factor is 0, i.e giving the widget a
    minimum of space.

    Permanently means that the widget may not be obscured by temporary
    messages. It is located at the far right of the status bar.

    \sa addPermanentWidget(), removeWidget(), addWidget()
*/
int QStatusBar::insertPermanentWidget(int index, QWidget *widget, int stretch)
{
    if (!widget)
        return -1;

    Q_D(QStatusBar);
    QStatusBarPrivate::SBItem item{widget, stretch, QStatusBarPrivate::Permanent};

    int idx = d->indexToLastNonPermanentWidget();
    if (Q_UNLIKELY(index < 0 || index > d->items.size() || (idx >= 0 && index <= idx))) {
        qWarning("QStatusBar::insertPermanentWidget: Index out of range (%d), appending widget", index);
        index = d->items.size();
    }
    d->items.insert(index, item);

    reformat();
    if (!widget->isHidden() || !widget->testAttribute(Qt::WA_WState_ExplicitShowHide))
        widget->show();

    return index;
}

/*!
    Removes the specified \a widget from the status bar.

    \note This function does not delete the widget but \e hides it.
    To add the widget again, you must call both the addWidget() and
    show() functions.

    \sa addWidget(), addPermanentWidget(), clearMessage()
*/

void QStatusBar::removeWidget(QWidget *widget)
{
    if (!widget)
        return;

    Q_D(QStatusBar);
    if (d->items.removeIf([widget](const auto &item) { return item.widget == widget; })) {
        widget->hide();
        reformat();
    }
#if defined(QT_DEBUG)
    else
        qDebug("QStatusBar::removeWidget(): Widget not found.");
#endif
}

/*!
    \property QStatusBar::sizeGripEnabled

    \brief whether the QSizeGrip in the bottom-right corner of the
    status bar is enabled

    The size grip is enabled by default.
*/

bool QStatusBar::isSizeGripEnabled() const
{
#if !QT_CONFIG(sizegrip)
    return false;
#else
    Q_D(const QStatusBar);
    return !!d->resizer;
#endif
}

void QStatusBar::setSizeGripEnabled(bool enabled)
{
#if !QT_CONFIG(sizegrip)
    Q_UNUSED(enabled);
#else
    Q_D(QStatusBar);
    if (!enabled != !d->resizer) {
        if (enabled) {
            d->resizer = new QSizeGrip(this);
            d->resizer->hide();
            d->resizer->installEventFilter(this);
            d->showSizeGrip = true;
        } else {
            delete d->resizer;
            d->resizer = nullptr;
            d->showSizeGrip = false;
        }
        reformat();
        if (d->resizer && isVisible())
            d->tryToShowSizeGrip();
    }
#endif
}


/*!
    Changes the status bar's appearance to account for item changes.

    Special subclasses may need this function, but geometry management
    will usually take care of any necessary rearrangements.
*/
void QStatusBar::reformat()
{
    Q_D(QStatusBar);
    if (d->box)
        delete d->box;

    QBoxLayout *vbox;
#if QT_CONFIG(sizegrip)
    if (d->resizer) {
        d->box = new QHBoxLayout(this);
        d->box->setContentsMargins(QMargins());
        vbox = new QVBoxLayout;
        d->box->addLayout(vbox);
    } else
#endif
    {
        vbox = d->box = new QVBoxLayout(this);
        d->box->setContentsMargins(QMargins());
    }
    vbox->addSpacing(3);
    QBoxLayout* l = new QHBoxLayout;
    vbox->addLayout(l);
    l->addSpacing(2);
    l->setSpacing(6);

    int maxH = fontMetrics().height();

    qsizetype i;
    for (i = 0; i < d->items.size(); ++i) {
        const auto &item = d->items.at(i);
        if (item.isPermanent())
            break;
        l->addWidget(item.widget, item.stretch);
        int itemH = qMin(qSmartMinSize(item.widget).height(), item.widget->maximumHeight());
        maxH = qMax(maxH, itemH);
    }

    l->addStretch(0);

    for (; i < d->items.size(); ++i) {
        const auto &item = d->items.at(i);
        l->addWidget(item.widget, item.stretch);
        int itemH = qMin(qSmartMinSize(item.widget).height(), item.widget->maximumHeight());
        maxH = qMax(maxH, itemH);
    }
#if QT_CONFIG(sizegrip)
    if (d->resizer) {
        maxH = qMax(maxH, d->resizer->sizeHint().height());
        d->box->addSpacing(1);
        d->box->addWidget(d->resizer, 0, Qt::AlignBottom);
    }
#endif
    l->addStrut(maxH);
    d->savedStrut = maxH;
    vbox->addSpacing(2);
    d->box->activate();
    update();
}

/*!

  Hides the normal status indications and displays the given \a
  message for the specified number of milli-seconds (\a{timeout}). If
  \a{timeout} is 0 (default), the \a {message} remains displayed until
  the clearMessage() slot is called or until the showMessage() slot is
  called again to change the message.

  Note that showMessage() is called to show temporary explanations of
  tool tip texts, so passing a \a{timeout} of 0 is not sufficient to
  display a \l{permanent message}{permanent message}.

    \sa messageChanged(), currentMessage(), clearMessage()
*/
void QStatusBar::showMessage(const QString &message, int timeout)
{
    Q_D(QStatusBar);

    if (timeout > 0) {
        if (!d->timer) {
            d->timer = new QTimer(this);
            connect(d->timer, SIGNAL(timeout()), this, SLOT(clearMessage()));
        }
        d->timer->start(timeout);
    } else if (d->timer) {
        delete d->timer;
        d->timer = nullptr;
    }
    if (d->tempItem == message)
        return;
    d->tempItem = message;

    hideOrShow();
}

/*!
    Removes any temporary message being shown.

    \sa currentMessage(), showMessage(), removeWidget()
*/

void QStatusBar::clearMessage()
{
    Q_D(QStatusBar);
    if (d->tempItem.isEmpty())
        return;
    if (d->timer) {
        qDeleteInEventHandler(d->timer);
        d->timer = nullptr;
    }
    d->tempItem.clear();
    hideOrShow();
}

/*!
    Returns the temporary message currently shown,
    or an empty string if there is no such message.

    \sa showMessage()
*/
QString QStatusBar::currentMessage() const
{
    Q_D(const QStatusBar);
    return d->tempItem;
}

/*!
    \fn void QStatusBar::messageChanged(const QString &message)

    This signal is emitted whenever the temporary status message
    changes. The new temporary message is passed in the \a message
    parameter which is a null-string when the message has been
    removed.

    \sa showMessage(), clearMessage()
*/

/*!
    Ensures that the right widgets are visible.

    Used by the showMessage() and clearMessage() functions.
*/
void QStatusBar::hideOrShow()
{
    Q_D(QStatusBar);
    bool haveMessage = !d->tempItem.isEmpty();

    for (const auto &item : std::as_const(d->items)) {
        if (item.isPermanent())
            break;
        if (haveMessage && item.widget->isVisible()) {
            item.widget->hide();
            item.widget->setAttribute(Qt::WA_WState_ExplicitShowHide, false);
        } else if (!haveMessage && !item.widget->testAttribute(Qt::WA_WState_ExplicitShowHide)) {
            item.widget->show();
        }
    }

    emit messageChanged(d->tempItem);

#if QT_CONFIG(accessibility)
    if (QAccessible::isActive()) {
        QAccessibleEvent event(this, QAccessible::NameChanged);
        QAccessible::updateAccessibility(&event);
    }
#endif

    update(d->messageRect());
}

/*!
  \reimp
 */
void QStatusBar::showEvent(QShowEvent *)
{
#if QT_CONFIG(sizegrip)
    Q_D(QStatusBar);
    if (d->resizer && d->showSizeGrip)
        d->tryToShowSizeGrip();
#endif
}

/*!
    \reimp
    \fn void QStatusBar::paintEvent(QPaintEvent *event)

    Shows the temporary message, if appropriate, in response to the
    paint \a event.
*/
void QStatusBar::paintEvent(QPaintEvent *event)
{
    Q_D(QStatusBar);
    bool haveMessage = !d->tempItem.isEmpty();

    QPainter p(this);
    QStyleOption opt;
    opt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_PanelStatusBar, &opt, &p, this);

    for (const auto &item : std::as_const(d->items)) {
        if (item.widget->isVisible() && (!haveMessage || item.isPermanent())) {
            QRect ir = item.widget->geometry().adjusted(-2, -1, 2, 1);
            if (event->rect().intersects(ir)) {
                QStyleOption opt(0);
                opt.rect = ir;
                opt.palette = palette();
                opt.state = QStyle::State_None;
                style()->drawPrimitive(QStyle::PE_FrameStatusBarItem, &opt, &p, item.widget);
            }
        }
    }
    if (haveMessage) {
        p.setPen(palette().windowText().color());
        p.drawText(d->messageRect(), Qt::AlignLeading | Qt::AlignVCenter | Qt::TextSingleLine, d->tempItem);
    }
}

/*!
    \reimp
*/
void QStatusBar::resizeEvent(QResizeEvent * e)
{
    QWidget::resizeEvent(e);
}

/*!
    \reimp
*/

bool QStatusBar::event(QEvent *e)
{
    Q_D(QStatusBar);

    switch (e->type()) {
    case QEvent::LayoutRequest: {
        // Calculate new strut height and call reformat() if it has changed
        int maxH = fontMetrics().height();

        for (const auto &item : std::as_const(d->items)) {
            const int itemH = qMin(qSmartMinSize(item.widget).height(), item.widget->maximumHeight());
            maxH = qMax(maxH, itemH);
        }

#if QT_CONFIG(sizegrip)
        if (d->resizer)
            maxH = qMax(maxH, d->resizer->sizeHint().height());
#endif

        if (maxH != d->savedStrut)
            reformat();
        else
            update();
        break;
    }
    case QEvent::ChildRemoved:
        for (int i = 0; i < d->items.size(); ++i) {
            const auto &item = d->items.at(i);
            if (item.widget == static_cast<QChildEvent *>(e)->child())
                d->items.removeAt(i);
        }
        break;
    default:
        break;
    }

    return QWidget::event(e);
}

QT_END_NAMESPACE

#include "moc_qstatusbar.cpp"
