// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwidgetresizehandler_p.h"

#include "qframe.h"
#include "qapplication.h"
#include "private/qwidget_p.h"
#include "qcursor.h"
#if QT_CONFIG(sizegrip)
#include "qsizegrip.h"
#endif
#include "qevent.h"
#include "qdebug.h"
#include "private/qlayoutengine_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#define RANGE 4

static bool resizeHorizontalDirectionFixed = false;
static bool resizeVerticalDirectionFixed = false;

QWidgetResizeHandler::QWidgetResizeHandler(QWidget *parent, QWidget *cw)
    : QObject(parent), widget(parent), childWidget(cw ? cw : parent),
      fw(0), extrahei(0), buttonDown(false), active(false)
{
    mode = Nowhere;
    widget->setMouseTracking(true);
    QFrame *frame = qobject_cast<QFrame*>(widget);
    range = frame ? frame->frameWidth() : RANGE;
    range = qMax(RANGE, range);
    enabled = true;
    widget->installEventFilter(this);
}

void QWidgetResizeHandler::setEnabled(bool b)
{
    if (b == enabled)
        return;

    enabled = b;
    if (!enabled)
        setMouseCursor(Nowhere);
}

bool QWidgetResizeHandler::isEnabled() const
{
    return enabled;
}

bool QWidgetResizeHandler::eventFilter(QObject *o, QEvent *ee)
{
    if (!isEnabled()
        || (ee->type() != QEvent::MouseButtonPress
            && ee->type() != QEvent::MouseButtonRelease
            && ee->type() != QEvent::MouseMove
            && ee->type() != QEvent::KeyPress
            && ee->type() != QEvent::ShortcutOverride)
        )
        return false;

    Q_ASSERT(o == widget);
    QWidget *w = widget;
    if (QApplication::activePopupWidget()) {
        if (buttonDown && ee->type() == QEvent::MouseButtonRelease)
            buttonDown = false;
        return false;
    }

    switch (ee->type()) {
    case QEvent::MouseButtonPress: {
        QMouseEvent *e = static_cast<QMouseEvent *>(ee);
        if (w->isMaximized())
            break;
        const QRect widgetRect = widget->rect().marginsAdded(QMargins(range, range, range, range));
        const QPoint cursorPoint = widget->mapFromGlobal(e->globalPosition().toPoint());
        if (!widgetRect.contains(cursorPoint))
            return false;
        if (e->button() == Qt::LeftButton) {
            buttonDown = false;
            emit activate();
            mouseMoveEvent(e);
            buttonDown = true;
            moveOffset = widget->mapFromGlobal(e->globalPosition().toPoint());
            invertedMoveOffset = widget->rect().bottomRight() - moveOffset;
            if (mode != Center)
                return true;
        }
    } break;
    case QEvent::MouseButtonRelease:
        if (w->isMaximized())
            break;
        if (static_cast<QMouseEvent *>(ee)->button() == Qt::LeftButton) {
            active = false;
            buttonDown = false;
            widget->releaseMouse();
            widget->releaseKeyboard();
            if (mode != Center)
                return true;
        }
        break;
    case QEvent::MouseMove: {
        if (w->isMaximized())
            break;
        QMouseEvent *e = static_cast<QMouseEvent *>(ee);
        buttonDown = buttonDown && (e->buttons() & Qt::LeftButton); // safety, state machine broken!
        mouseMoveEvent(e);
        if (mode != Center)
            return true;
    } break;
    case QEvent::KeyPress:
        keyPressEvent(static_cast<QKeyEvent *>(ee));
        break;
    case QEvent::ShortcutOverride:
        buttonDown &= ((QGuiApplication::mouseButtons() & Qt::LeftButton) != Qt::NoButton);
        if (buttonDown) {
            ee->accept();
            return true;
        }
        break;
    default:
        break;
    }

    return false;
}

void QWidgetResizeHandler::mouseMoveEvent(QMouseEvent *e)
{
    QPoint pos = widget->mapFromGlobal(e->globalPosition().toPoint());
    if (!active && !buttonDown) {
        if (pos.y() <= range && pos.x() <= range)
            mode = TopLeft;
        else if (pos.y() >= widget->height()-range && pos.x() >= widget->width()-range)
            mode = BottomRight;
        else if (pos.y() >= widget->height()-range && pos.x() <= range)
            mode = BottomLeft;
        else if (pos.y() <= range && pos.x() >= widget->width()-range)
            mode = TopRight;
        else if (pos.y() <= range)
            mode = Top;
        else if (pos.y() >= widget->height()-range)
            mode = Bottom;
        else if (pos.x() <= range)
            mode = Left;
        else if ( pos.x() >= widget->width()-range)
            mode = Right;
        else if (widget->rect().contains(pos))
            mode = Center;
        else
            mode = Nowhere;

        if (widget->isMinimized() || !isEnabled())
            mode = Center;
#ifndef QT_NO_CURSOR
        setMouseCursor(mode);
#endif
        return;
    }

    if (mode == Center)
        return;

    if (widget->testAttribute(Qt::WA_WState_ConfigPending))
        return;


    QPoint globalPos = (!widget->isWindow() && widget->parentWidget()) ?
                       widget->parentWidget()->mapFromGlobal(e->globalPosition().toPoint()) : e->globalPosition().toPoint();
    if (!widget->isWindow() && !widget->parentWidget()->rect().contains(globalPos)) {
        if (globalPos.x() < 0)
            globalPos.rx() = 0;
        if (globalPos.y() < 0)
            globalPos.ry() = 0;
        if (globalPos.x() > widget->parentWidget()->width())
            globalPos.rx() = widget->parentWidget()->width();
        if (globalPos.y() > widget->parentWidget()->height())
            globalPos.ry() = widget->parentWidget()->height();
    }

    QPoint p = globalPos + invertedMoveOffset;
    QPoint pp = globalPos - moveOffset;

    // Workaround for window managers which refuse to move a tool window partially offscreen.
    if (QGuiApplication::platformName() == "xcb"_L1) {
        const QRect desktop = QWidgetPrivate::availableScreenGeometry(widget);
        pp.rx() = qMax(pp.x(), desktop.left());
        pp.ry() = qMax(pp.y(), desktop.top());
        p.rx() = qMin(p.x(), desktop.right());
        p.ry() = qMin(p.y(), desktop.bottom());
    }

    QSize ms = qSmartMinSize(childWidget);
    int mw = ms.width();
    int mh = ms.height();
    if (childWidget != widget) {
        mw += 2 * fw;
        mh += 2 * fw + extrahei;
    }

    QSize maxsize(childWidget->maximumSize());
    if (childWidget != widget)
        maxsize += QSize(2 * fw, 2 * fw + extrahei);
    QSize mpsize(widget->geometry().right() - pp.x() + 1,
                  widget->geometry().bottom() - pp.y() + 1);
    mpsize = mpsize.expandedTo(widget->minimumSize()).expandedTo(QSize(mw, mh))
                    .boundedTo(maxsize);
    QPoint mp(widget->geometry().right() - mpsize.width() + 1,
               widget->geometry().bottom() - mpsize.height() + 1);

    QRect geom = widget->geometry();

    switch (mode) {
    case TopLeft:
        geom = QRect(mp, widget->geometry().bottomRight()) ;
        break;
    case BottomRight:
        geom = QRect(widget->geometry().topLeft(), p) ;
        break;
    case BottomLeft:
        geom = QRect(QPoint(mp.x(), widget->geometry().y()), QPoint(widget->geometry().right(), p.y())) ;
        break;
    case TopRight:
        geom = QRect(QPoint(widget->geometry().x(), mp.y()), QPoint(p.x(), widget->geometry().bottom())) ;
        break;
    case Top:
        geom = QRect(QPoint(widget->geometry().left(), mp.y()), widget->geometry().bottomRight()) ;
        break;
    case Bottom:
        geom = QRect(widget->geometry().topLeft(), QPoint(widget->geometry().right(), p.y())) ;
        break;
    case Left:
        geom = QRect(QPoint(mp.x(), widget->geometry().top()), widget->geometry().bottomRight()) ;
        break;
    case Right:
        geom = QRect(widget->geometry().topLeft(), QPoint(p.x(), widget->geometry().bottom())) ;
        break;
    default:
        break;
    }

    geom = QRect(geom.topLeft(),
                  geom.size().expandedTo(widget->minimumSize())
                             .expandedTo(QSize(mw, mh))
                             .boundedTo(maxsize));

    if (geom != widget->geometry() &&
        (widget->isWindow() || widget->parentWidget()->rect().intersects(geom))) {
        widget->setGeometry(geom);
    }
}

void QWidgetResizeHandler::setMouseCursor(MousePosition m)
{
#ifdef QT_NO_CURSOR
    Q_UNUSED(m);
#else
    QObjectList children = widget->children();
    for (int i = 0; i < children.size(); ++i) {
        if (QWidget *w = qobject_cast<QWidget*>(children.at(i))) {
            if (!w->testAttribute(Qt::WA_SetCursor)) {
                w->setCursor(Qt::ArrowCursor);
            }
        }
    }

    switch (m) {
    case TopLeft:
    case BottomRight:
        widget->setCursor(Qt::SizeFDiagCursor);
        break;
    case BottomLeft:
    case TopRight:
        widget->setCursor(Qt::SizeBDiagCursor);
        break;
    case Top:
    case Bottom:
        widget->setCursor(Qt::SizeVerCursor);
        break;
    case Left:
    case Right:
        widget->setCursor(Qt::SizeHorCursor);
        break;
    default:
        widget->setCursor(Qt::ArrowCursor);
        break;
    }
#endif // QT_NO_CURSOR
}

void QWidgetResizeHandler::keyPressEvent(QKeyEvent * e)
{
    if (!isResizing())
        return;
    bool is_control = e->modifiers() & Qt::ControlModifier;
    int delta = is_control?1:8;
    QPoint pos = QCursor::pos();
    switch (e->key()) {
    case Qt::Key_Left:
        pos.rx() -= delta;
        if (pos.x() <= QGuiApplication::primaryScreen()->virtualGeometry().left()) {
            if (mode == TopLeft || mode == BottomLeft) {
                moveOffset.rx() += delta;
                invertedMoveOffset.rx() += delta;
            } else {
                moveOffset.rx() -= delta;
                invertedMoveOffset.rx() -= delta;
            }
        }
        if (isResizing() && !resizeHorizontalDirectionFixed) {
            resizeHorizontalDirectionFixed = true;
            if (mode == BottomRight)
                mode = BottomLeft;
            else if (mode == TopRight)
                mode = TopLeft;
#ifndef QT_NO_CURSOR
            setMouseCursor(mode);
            widget->grabMouse(widget->cursor());
#else
            widget->grabMouse();
#endif
        }
        break;
    case Qt::Key_Right:
        pos.rx() += delta;
        if (pos.x() >= QGuiApplication::primaryScreen()->virtualGeometry().right()) {
            if (mode == TopRight || mode == BottomRight) {
                moveOffset.rx() += delta;
                invertedMoveOffset.rx() += delta;
            } else {
                moveOffset.rx() -= delta;
                invertedMoveOffset.rx() -= delta;
            }
        }
        if (isResizing() && !resizeHorizontalDirectionFixed) {
            resizeHorizontalDirectionFixed = true;
            if (mode == BottomLeft)
                mode = BottomRight;
            else if (mode == TopLeft)
                mode = TopRight;
#ifndef QT_NO_CURSOR
            setMouseCursor(mode);
            widget->grabMouse(widget->cursor());
#else
            widget->grabMouse();
#endif
        }
        break;
    case Qt::Key_Up:
        pos.ry() -= delta;
        if (pos.y() <= QGuiApplication::primaryScreen()->virtualGeometry().top()) {
            if (mode == TopLeft || mode == TopRight) {
                moveOffset.ry() += delta;
                invertedMoveOffset.ry() += delta;
            } else {
                moveOffset.ry() -= delta;
                invertedMoveOffset.ry() -= delta;
            }
        }
        if (isResizing() && !resizeVerticalDirectionFixed) {
            resizeVerticalDirectionFixed = true;
            if (mode == BottomLeft)
                mode = TopLeft;
            else if (mode == BottomRight)
                mode = TopRight;
#ifndef QT_NO_CURSOR
            setMouseCursor(mode);
            widget->grabMouse(widget->cursor());
#else
            widget->grabMouse();
#endif
        }
        break;
    case Qt::Key_Down:
        pos.ry() += delta;
        if (pos.y() >= QGuiApplication::primaryScreen()->virtualGeometry().bottom()) {
            if (mode == BottomLeft || mode == BottomRight) {
                moveOffset.ry() += delta;
                invertedMoveOffset.ry() += delta;
            } else {
                moveOffset.ry() -= delta;
                invertedMoveOffset.ry() -= delta;
            }
        }
        if (isResizing() && !resizeVerticalDirectionFixed) {
            resizeVerticalDirectionFixed = true;
            if (mode == TopLeft)
                mode = BottomLeft;
            else if (mode == TopRight)
                mode = BottomRight;
#ifndef QT_NO_CURSOR
            setMouseCursor(mode);
            widget->grabMouse(widget->cursor());
#else
            widget->grabMouse();
#endif
        }
        break;
    case Qt::Key_Space:
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Escape:
        active = false;
        widget->releaseMouse();
        widget->releaseKeyboard();
        buttonDown = false;
        break;
    default:
        return;
    }
    QCursor::setPos(pos);
}


void QWidgetResizeHandler::doResize()
{
    if (!enabled)
        return;

    active = true;
    moveOffset = widget->mapFromGlobal(QCursor::pos());
    if (moveOffset.x() < widget->width()/2) {
        if (moveOffset.y() < widget->height()/2)
            mode = TopLeft;
        else
            mode = BottomLeft;
    } else {
        if (moveOffset.y() < widget->height()/2)
            mode = TopRight;
        else
            mode = BottomRight;
    }
    invertedMoveOffset = widget->rect().bottomRight() - moveOffset;
#ifndef QT_NO_CURSOR
    setMouseCursor(mode);
    widget->grabMouse(widget->cursor() );
#else
    widget->grabMouse();
#endif
    widget->grabKeyboard();
    resizeHorizontalDirectionFixed = false;
    resizeVerticalDirectionFixed = false;
}

QT_END_NAMESPACE

#include "moc_qwidgetresizehandler_p.cpp"
