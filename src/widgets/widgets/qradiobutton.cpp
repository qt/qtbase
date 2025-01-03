// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qradiobutton.h"
#include "qapplication.h"
#include "qbitmap.h"
#if QT_CONFIG(buttongroup)
#include "qbuttongroup.h"
#endif
#include "qstylepainter.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qevent.h"

#include "private/qabstractbutton_p.h"

QT_BEGIN_NAMESPACE

class QRadioButtonPrivate : public QAbstractButtonPrivate
{
    Q_DECLARE_PUBLIC(QRadioButton)

public:
    QRadioButtonPrivate() : QAbstractButtonPrivate(QSizePolicy::RadioButton), hovering(true) {}
    void init();
    uint hovering : 1;
};

/*
    Initializes the radio button.
*/
void QRadioButtonPrivate::init()
{
    Q_Q(QRadioButton);
    q->setCheckable(true);
    q->setAutoExclusive(true);
    q->setMouseTracking(true);
    q->setForegroundRole(QPalette::WindowText);
    q->setAttribute(Qt::WA_MacShowFocusRect);
    setLayoutItemMargins(QStyle::SE_RadioButtonLayoutItem);
}

/*!
    \class QRadioButton
    \brief The QRadioButton widget provides a radio button with a text label.

    \ingroup basicwidgets
    \inmodule QtWidgets

    \image windows-radiobutton.png

    A QRadioButton is an option button that can be switched on (checked) or
    off (unchecked). Radio buttons typically present the user with a "one
    of many" choice. In a group of radio buttons, only one radio button at
    a time can be checked; if the user selects another button, the
    previously selected button is switched off.

    Radio buttons are autoExclusive by default. If auto-exclusive is
    enabled, radio buttons that belong to the same parent widget
    behave as if they were part of the same exclusive button group. If
    you need multiple exclusive button groups for radio buttons that
    belong to the same parent widget, put them into a QButtonGroup.

    Whenever a button is switched on or off, it emits the toggled() signal.
    Connect to this signal if you want to trigger an action each time the
    button changes state. Use isChecked() to see if a particular button is
    selected.

    Just like QPushButton, a radio button displays text, and
    optionally a small icon. The icon is set with setIcon(). The text
    can be set in the constructor or with setText(). A shortcut key
    can be specified by preceding the preferred character with an
    ampersand in the text. For example:

    \snippet code/src_gui_widgets_qradiobutton.cpp 0

    In this example the shortcut is \e{Alt+c}. See the \l
    {QShortcut#mnemonic}{QShortcut} documentation for details. To
    display an actual ampersand, use '&&'.

    Important inherited members: text(), setText(), text(),
    setDown(), isDown(), autoRepeat(), group(), setAutoRepeat(),
    toggle(), pressed(), released(), clicked(), and toggled().

    \sa QPushButton, QToolButton, QCheckBox, {Group Box Example}
*/


/*!
    Constructs a radio button with the given \a parent, but with no text or
    pixmap.

    The \a parent argument is passed on to the QAbstractButton constructor.
*/

QRadioButton::QRadioButton(QWidget *parent)
    : QAbstractButton(*new QRadioButtonPrivate, parent)
{
    Q_D(QRadioButton);
    d->init();
}

/*!
    Destructor.
*/
QRadioButton::~QRadioButton()
{
}

/*!
    Constructs a radio button with the given \a parent and \a text string.

    The \a parent argument is passed on to the QAbstractButton constructor.
*/

QRadioButton::QRadioButton(const QString &text, QWidget *parent)
    : QRadioButton(parent)
{
    setText(text);
}

/*!
    Initialize \a option with the values from this QRadioButton. This method is useful
    for subclasses when they need a QStyleOptionButton, but don't want to fill
    in all the information themselves.

    \sa QStyleOption::initFrom()
*/
void QRadioButton::initStyleOption(QStyleOptionButton *option) const
{
    if (!option)
        return;
    Q_D(const QRadioButton);
    option->initFrom(this);
    option->text = d->text;
    option->icon = d->icon;
    option->iconSize = iconSize();
    if (d->down)
        option->state |= QStyle::State_Sunken;
    option->state |= (d->checked) ? QStyle::State_On : QStyle::State_Off;
    if (testAttribute(Qt::WA_Hover) && underMouse()) {
        option->state.setFlag(QStyle::State_MouseOver, d->hovering);
    }
}

/*!
    \reimp
*/
QSize QRadioButton::sizeHint() const
{
    Q_D(const QRadioButton);
    if (d->sizeHint.isValid())
        return d->sizeHint;
    ensurePolished();
    QStyleOptionButton opt;
    initStyleOption(&opt);
    QSize sz = style()->itemTextRect(fontMetrics(), QRect(), Qt::TextShowMnemonic,
                                     false, text()).size();
    if (!opt.icon.isNull())
        sz = QSize(sz.width() + opt.iconSize.width() + 4, qMax(sz.height(), opt.iconSize.height()));
    d->sizeHint = style()->sizeFromContents(QStyle::CT_RadioButton, &opt, sz, this);
    return d->sizeHint;
}

/*!
    \reimp
*/
QSize QRadioButton::minimumSizeHint() const
{
    return sizeHint();
}

/*!
    \reimp
*/
bool QRadioButton::hitButton(const QPoint &pos) const
{
    QStyleOptionButton opt;
    initStyleOption(&opt);
    return style()->subElementRect(QStyle::SE_RadioButtonClickRect, &opt, this).contains(pos);
}

/*!
    \reimp
*/
void QRadioButton::mouseMoveEvent(QMouseEvent *e)
{
    Q_D(QRadioButton);
    if (testAttribute(Qt::WA_Hover)) {
        bool hit = false;
        if (underMouse())
            hit = hitButton(e->position().toPoint());

        if (hit != d->hovering) {
            update();
            d->hovering = hit;
        }
    }

    QAbstractButton::mouseMoveEvent(e);
}

/*!\reimp
 */
void QRadioButton::paintEvent(QPaintEvent *)
{
    QStylePainter p(this);
    QStyleOptionButton opt;
    initStyleOption(&opt);
    p.drawControl(QStyle::CE_RadioButton, opt);
}

/*! \reimp */
bool QRadioButton::event(QEvent *e)
{
    Q_D(QRadioButton);
    if (e->type() == QEvent::StyleChange
#ifdef Q_OS_MAC
            || e->type() == QEvent::MacSizeChange
#endif
            )
        d->setLayoutItemMargins(QStyle::SE_RadioButtonLayoutItem);
    return QAbstractButton::event(e);
}


QT_END_NAMESPACE

#include "moc_qradiobutton.cpp"
