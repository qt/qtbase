// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qslider.h"
#if QT_CONFIG(accessibility)
#include "qaccessible.h"
#endif
#include "qapplication.h"
#include "qevent.h"
#include "qpainter.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qstylepainter.h"
#include "private/qapplication_p.h"
#include "private/qabstractslider_p.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

class QSliderPrivate : public QAbstractSliderPrivate
{
    Q_DECLARE_PUBLIC(QSlider)
public:
    QStyle::SubControl pressedControl;
    int tickInterval;
    QSlider::TickPosition tickPosition;
    int clickOffset;
    void init();
    void resetLayoutItemMargins();
    int pixelPosToRangeValue(int pos) const;
    inline int pick(const QPoint &pt) const;

    QStyle::SubControl newHoverControl(const QPoint &pos);
    bool updateHoverControl(const QPoint &pos);
    QStyle::SubControl hoverControl;
    QRect hoverRect;
};

void QSliderPrivate::init()
{
    Q_Q(QSlider);
    pressedControl = QStyle::SC_None;
    tickInterval = 0;
    tickPosition = QSlider::NoTicks;
    hoverControl = QStyle::SC_None;
    q->setFocusPolicy(Qt::FocusPolicy(q->style()->styleHint(QStyle::SH_Button_FocusPolicy, nullptr, q)));
    QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Fixed, QSizePolicy::Slider);
    if (orientation == Qt::Vertical)
        sp.transpose();
    q->setSizePolicy(sp);
    q->setAttribute(Qt::WA_WState_OwnSizePolicy, false);
    resetLayoutItemMargins();
}

void QSliderPrivate::resetLayoutItemMargins()
{
    Q_Q(QSlider);
    QStyleOptionSlider opt;
    // ### This is (also) reached from the ctor which is unfortunate since a possible
    // ### re-implementation of initStyleOption is then not called.
    q->initStyleOption(&opt);
    setLayoutItemMargins(QStyle::SE_SliderLayoutItem, &opt);
}

int QSliderPrivate::pixelPosToRangeValue(int pos) const
{
    Q_Q(const QSlider);
    QStyleOptionSlider opt;
    q->initStyleOption(&opt);
    QRect gr = q->style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, q);
    QRect sr = q->style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, q);
    int sliderMin, sliderMax, sliderLength;

    if (orientation == Qt::Horizontal) {
        sliderLength = sr.width();
        sliderMin = gr.x();
        sliderMax = gr.right() - sliderLength + 1;
    } else {
        sliderLength = sr.height();
        sliderMin = gr.y();
        sliderMax = gr.bottom() - sliderLength + 1;
    }
    return QStyle::sliderValueFromPosition(minimum, maximum, pos - sliderMin,
                                           sliderMax - sliderMin, opt.upsideDown);
}

inline int QSliderPrivate::pick(const QPoint &pt) const
{
    return orientation == Qt::Horizontal ? pt.x() : pt.y();
}

/*!
    Initialize \a option with the values from this QSlider. This method
    is useful for subclasses when they need a QStyleOptionSlider, but don't want
    to fill in all the information themselves.

    \sa QStyleOption::initFrom()
*/
void QSlider::initStyleOption(QStyleOptionSlider *option) const
{
    if (!option)
        return;

    Q_D(const QSlider);
    option->initFrom(this);
    option->subControls = QStyle::SC_None;
    option->activeSubControls = QStyle::SC_None;
    option->orientation = d->orientation;
    option->maximum = d->maximum;
    option->minimum = d->minimum;
    option->tickPosition = (QSlider::TickPosition)d->tickPosition;
    option->tickInterval = d->tickInterval;
    option->upsideDown = (d->orientation == Qt::Horizontal) ?
                     (d->invertedAppearance != (option->direction == Qt::RightToLeft))
                     : (!d->invertedAppearance);
    option->direction = Qt::LeftToRight; // we use the upsideDown option instead
    option->sliderPosition = d->position;
    option->sliderValue = d->value;
    option->singleStep = d->singleStep;
    option->pageStep = d->pageStep;
    if (d->orientation == Qt::Horizontal)
        option->state |= QStyle::State_Horizontal;

    if (d->pressedControl) {
        option->activeSubControls = d->pressedControl;
        option->state |= QStyle::State_Sunken;
    } else {
        option->activeSubControls = d->hoverControl;
    }
}

bool QSliderPrivate::updateHoverControl(const QPoint &pos)
{
    Q_Q(QSlider);
    QRect lastHoverRect = hoverRect;
    QStyle::SubControl lastHoverControl = hoverControl;
    bool doesHover = q->testAttribute(Qt::WA_Hover);
    if (lastHoverControl != newHoverControl(pos) && doesHover) {
        q->update(lastHoverRect);
        q->update(hoverRect);
        return true;
    }
    return !doesHover;
}

QStyle::SubControl QSliderPrivate::newHoverControl(const QPoint &pos)
{
    Q_Q(QSlider);
    QStyleOptionSlider opt;
    q->initStyleOption(&opt);
    opt.subControls = QStyle::SC_All;
    QRect handleRect = q->style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, q);
    QRect grooveRect = q->style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, q);
    QRect tickmarksRect = q->style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderTickmarks, q);

    if (handleRect.contains(pos)) {
        hoverRect = handleRect;
        hoverControl = QStyle::SC_SliderHandle;
    } else if (grooveRect.contains(pos)) {
        hoverRect = grooveRect;
        hoverControl = QStyle::SC_SliderGroove;
    } else if (tickmarksRect.contains(pos)) {
        hoverRect = tickmarksRect;
        hoverControl = QStyle::SC_SliderTickmarks;
    } else {
        hoverRect = QRect();
        hoverControl = QStyle::SC_None;
    }

    return hoverControl;
}

/*!
    \class QSlider
    \brief The QSlider widget provides a vertical or horizontal slider.

    \ingroup basicwidgets
    \inmodule QtWidgets

    \image fusion-slider.png

    The slider is the classic widget for controlling a bounded value.
    It lets the user move a slider handle along a horizontal or vertical
    groove and translates the handle's position into an integer value
    within the legal range.

    QSlider has very few of its own functions; most of the functionality is in
    QAbstractSlider. The most useful functions are setValue() to set
    the slider directly to some value; triggerAction() to simulate
    the effects of clicking (useful for shortcut keys);
    setSingleStep(), setPageStep() to set the steps; and setMinimum()
    and setMaximum() to define the range of the scroll bar.

    QSlider provides methods for controlling tickmarks.  You can use
    setTickPosition() to indicate where you want the tickmarks to be,
    setTickInterval() to indicate how many of them you want. the
    currently set tick position and interval can be queried using the
    tickPosition() and tickInterval() functions, respectively.

    QSlider inherits a comprehensive set of signals:
    \table
    \header \li Signal \li Description
    \row \li \l valueChanged()
    \li Emitted when the slider's value has changed. The tracking()
       determines whether this signal is emitted during user
       interaction.
    \row \li \l sliderPressed()
    \li Emitted when the user starts to drag the slider.
    \row \li \l sliderMoved()
    \li Emitted when the user drags the slider.
    \row \li \l sliderReleased()
    \li Emitted when the user releases the slider.
    \endtable

    QSlider only provides integer ranges. Note that although
    QSlider handles very large numbers, it becomes difficult for users
    to use a slider accurately for very large ranges.

    A slider accepts focus on Tab and provides both a mouse wheel and a
    keyboard interface. The keyboard interface is the following:

    \list
        \li Left/Right move a horizontal slider by one single step.
        \li Up/Down move a vertical slider by one single step.
        \li PageUp moves up one page.
        \li PageDown moves down one page.
        \li Home moves to the start (minimum).
        \li End moves to the end (maximum).
    \endlist

    \sa QScrollBar, QSpinBox, QDial, {Sliders Example}
*/


/*!
    \enum QSlider::TickPosition

    This enum specifies where the tick marks are to be drawn relative
    to the slider's groove and the handle the user moves.

    \value NoTicks Do not draw any tick marks.
    \value TicksBothSides Draw tick marks on both sides of the groove.
    \value TicksAbove Draw tick marks above the (horizontal) slider
    \value TicksBelow Draw tick marks below the (horizontal) slider
    \value TicksLeft Draw tick marks to the left of the (vertical) slider
    \value TicksRight Draw tick marks to the right of the (vertical) slider
*/


/*!
    Constructs a vertical slider with the given \a parent.
*/
QSlider::QSlider(QWidget *parent)
    : QSlider(Qt::Vertical, parent)
{
}

/*!
    Constructs a slider with the given \a parent. The \a orientation
    parameter determines whether the slider is horizontal or vertical;
    the valid values are Qt::Vertical and Qt::Horizontal.
*/

QSlider::QSlider(Qt::Orientation orientation, QWidget *parent)
    : QAbstractSlider(*new QSliderPrivate, parent)
{
    d_func()->orientation = orientation;
    d_func()->init();
}


/*!
    Destroys this slider.
*/
QSlider::~QSlider()
{
}

/*!
    \reimp
*/
void QSlider::paintEvent(QPaintEvent *)
{
    Q_D(QSlider);
    QStylePainter p(this);
    QStyleOptionSlider opt;
    initStyleOption(&opt);

    opt.subControls = QStyle::SC_SliderGroove | QStyle::SC_SliderHandle;
    if (d->tickPosition != NoTicks)
        opt.subControls |= QStyle::SC_SliderTickmarks;

    p.drawComplexControl(QStyle::CC_Slider, opt);
}

/*!
    \reimp
*/

bool QSlider::event(QEvent *event)
{
    Q_D(QSlider);

    switch(event->type()) {
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::HoverMove:
        if (const QHoverEvent *he = static_cast<const QHoverEvent *>(event))
            d->updateHoverControl(he->position().toPoint());
        break;
    case QEvent::StyleChange:
    case QEvent::MacSizeChange:
        d->resetLayoutItemMargins();
        break;
    default:
        break;
    }
    return QAbstractSlider::event(event);
}

/*!
    \reimp
*/
void QSlider::mousePressEvent(QMouseEvent *ev)
{
    Q_D(QSlider);
    if (d->maximum == d->minimum || (ev->buttons() ^ ev->button())) {
        ev->ignore();
        return;
    }
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplicationPrivate::keypadNavigationEnabled())
        setEditFocus(true);
#endif
    ev->accept();
    if ((ev->button() & style()->styleHint(QStyle::SH_Slider_AbsoluteSetButtons, nullptr, this)) == ev->button()) {
        QStyleOptionSlider opt;
        initStyleOption(&opt);
        const QRect sliderRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
        const QPoint center = sliderRect.center() - sliderRect.topLeft();
        // to take half of the slider off for the setSliderPosition call we use the center - topLeft

        setSliderPosition(d->pixelPosToRangeValue(d->pick(ev->position().toPoint() - center)));
        triggerAction(SliderMove);
        setRepeatAction(SliderNoAction);
        d->pressedControl = QStyle::SC_SliderHandle;
        update();
    } else if ((ev->button() & style()->styleHint(QStyle::SH_Slider_PageSetButtons, nullptr, this)) == ev->button()) {
        QStyleOptionSlider opt;
        initStyleOption(&opt);
        d->pressedControl = style()->hitTestComplexControl(QStyle::CC_Slider,
                                                           &opt, ev->position().toPoint(), this);
        SliderAction action = SliderNoAction;
        if (d->pressedControl == QStyle::SC_SliderGroove) {
            const QRect sliderRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
            int pressValue = d->pixelPosToRangeValue(d->pick(ev->position().toPoint() - sliderRect.center() + sliderRect.topLeft()));
            d->pressValue = pressValue;
            if (pressValue > d->value)
                action = SliderPageStepAdd;
            else if (pressValue < d->value)
                action = SliderPageStepSub;
            if (action) {
                triggerAction(action);
                setRepeatAction(action);
            }
        }
    } else {
        ev->ignore();
        return;
    }

    if (d->pressedControl == QStyle::SC_SliderHandle) {
        QStyleOptionSlider opt;
        initStyleOption(&opt);
        setRepeatAction(SliderNoAction);
        QRect sr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
        d->clickOffset = d->pick(ev->position().toPoint() - sr.topLeft());
        update(sr);
        setSliderDown(true);
    }
}

/*!
    \reimp
*/
void QSlider::mouseMoveEvent(QMouseEvent *ev)
{
    Q_D(QSlider);
    if (d->pressedControl != QStyle::SC_SliderHandle) {
        ev->ignore();
        return;
    }
    ev->accept();
    int newPosition = d->pixelPosToRangeValue(d->pick(ev->position().toPoint()) - d->clickOffset);
    setSliderPosition(newPosition);
}


/*!
    \reimp
*/
void QSlider::mouseReleaseEvent(QMouseEvent *ev)
{
    Q_D(QSlider);
    if (d->pressedControl == QStyle::SC_None || ev->buttons()) {
        ev->ignore();
        return;
    }
    ev->accept();
    QStyle::SubControl oldPressed = QStyle::SubControl(d->pressedControl);
    d->pressedControl = QStyle::SC_None;
    setRepeatAction(SliderNoAction);
    if (oldPressed == QStyle::SC_SliderHandle)
        setSliderDown(false);
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    opt.subControls = oldPressed;
    update(style()->subControlRect(QStyle::CC_Slider, &opt, oldPressed, this));
}

/*!
    \reimp
*/
QSize QSlider::sizeHint() const
{
    Q_D(const QSlider);
    ensurePolished();
    const int SliderLength = 84, TickSpace = 5;
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    int thick = style()->pixelMetric(QStyle::PM_SliderThickness, &opt, this);
    if (d->tickPosition & TicksAbove)
        thick += TickSpace;
    if (d->tickPosition & TicksBelow)
        thick += TickSpace;
    int w = thick, h = SliderLength;
    if (d->orientation == Qt::Horizontal) {
        w = SliderLength;
        h = thick;
    }
    return style()->sizeFromContents(QStyle::CT_Slider, &opt, QSize(w, h), this);
}

/*!
    \reimp
*/
QSize QSlider::minimumSizeHint() const
{
    Q_D(const QSlider);
    QSize s = sizeHint();
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    int length = style()->pixelMetric(QStyle::PM_SliderLength, &opt, this);
    if (d->orientation == Qt::Horizontal)
        s.setWidth(length);
    else
        s.setHeight(length);
    return s;
}

/*!
    \property QSlider::tickPosition
    \brief the tickmark position for this slider

    The valid values are described by the QSlider::TickPosition enum.

    The default value is \l QSlider::NoTicks.

    \sa tickInterval
*/

void QSlider::setTickPosition(TickPosition position)
{
    Q_D(QSlider);
    d->tickPosition = position;
    d->resetLayoutItemMargins();
    update();
    updateGeometry();
}

QSlider::TickPosition QSlider::tickPosition() const
{
    return d_func()->tickPosition;
}

/*!
    \property QSlider::tickInterval
    \brief the interval between tickmarks

    This is a value interval, not a pixel interval. If it is 0, the
    slider will choose between singleStep and pageStep.

    The default value is 0.

    \sa tickPosition, singleStep, pageStep
*/

void QSlider::setTickInterval(int ts)
{
    d_func()->tickInterval = qMax(0, ts);
    update();
}

int QSlider::tickInterval() const
{
    return d_func()->tickInterval;
}

Q_WIDGETS_EXPORT QStyleOptionSlider qt_qsliderStyleOption(QSlider *slider)
{
    QStyleOptionSlider sliderOption;
    slider->initStyleOption(&sliderOption);
    return sliderOption;
}

QT_END_NAMESPACE

#include "moc_qslider.cpp"
