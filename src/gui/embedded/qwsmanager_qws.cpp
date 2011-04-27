/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwsmanager_qws.h"

#ifndef QT_NO_QWS_MANAGER

#include "qdrawutil.h"
#include "qapplication.h"
#include "qstyle.h"
#include "qwidget.h"
#include "qmenu.h"
#include "qpainter.h"
#include "private/qpainter_p.h"
#include "qregion.h"
#include "qevent.h"
#include "qcursor.h"
#include "qwsdisplay_qws.h"
#include "qdesktopwidget.h"

#include <private/qapplication_p.h>
#include <private/qwidget_p.h>
#include <private/qbackingstore_p.h>
#include <private/qwindowsurface_qws_p.h>
#include "qdecorationfactory_qws.h"

#include "qlayout.h"

#include "qwsmanager_p.h"

#include <qdebug.h>

QT_BEGIN_NAMESPACE

QWidget *QWSManagerPrivate::active = 0;
QPoint QWSManagerPrivate::mousePos;


QWSManagerPrivate::QWSManagerPrivate()
    : QObjectPrivate(), activeRegion(QDecoration::None), managed(0), popup(0),
      previousRegionType(0), previousRegionRepainted(false), entireDecorationNeedsRepaint(false)
{
    cached_region.regionType = 0;
}

QRegion &QWSManager::cachedRegion()
{
    return d_func()->cached_region.region;
}

/*!
    \class QWSManager
    \ingroup qws
    \internal
*/

/*!

*/
QWSManager::QWSManager(QWidget *w)
    : QObject(*new QWSManagerPrivate, (QObject*)0)
{
    d_func()->managed = w;

}

QWSManager::~QWSManager()
{
    Q_D(QWSManager);
#ifndef QT_NO_MENU
    if (d->popup)
        delete d->popup;
#endif
    if (d->managed == QWSManagerPrivate::active)
        QWSManagerPrivate::active = 0;
}

QWidget *QWSManager::widget()
{
    Q_D(QWSManager);
    return d->managed;
}

QWidget *QWSManager::grabbedMouse()
{
    return QWSManagerPrivate::active;
}

QRegion QWSManager::region()
{
    Q_D(QWSManager);
    return QApplication::qwsDecoration().region(d->managed, d->managed->geometry());
}

bool QWSManager::event(QEvent *e)
{
    if (QObject::event(e))
        return true;

    switch (e->type()) {
        case QEvent::MouseMove:
            mouseMoveEvent((QMouseEvent*)e);
            break;

        case QEvent::MouseButtonPress:
            mousePressEvent((QMouseEvent*)e);
            break;

        case QEvent::MouseButtonRelease:
            mouseReleaseEvent((QMouseEvent*)e);
            break;

        case QEvent::MouseButtonDblClick:
            mouseDoubleClickEvent((QMouseEvent*)e);
            break;

        case QEvent::Paint:
            paintEvent((QPaintEvent*)e);
            break;

        default:
            return false;
            break;
    }

    return true;
}

void QWSManager::mousePressEvent(QMouseEvent *e)
{
    Q_D(QWSManager);
    d->mousePos = e->globalPos();
    d->activeRegion = QApplication::qwsDecoration().regionAt(d->managed, d->mousePos);
    if(d->cached_region.regionType)
        d->previousRegionRepainted |= repaintRegion(d->cached_region.regionType, QDecoration::Pressed);

    if (d->activeRegion == QDecoration::Menu) {
        QPoint pos = (QApplication::layoutDirection() == Qt::LeftToRight
                      ? d->managed->geometry().topLeft()
                      : d->managed->geometry().topRight());
        menu(pos);
    }
    if (d->activeRegion != QDecoration::None &&
         d->activeRegion != QDecoration::Menu) {
        d->active = d->managed;
        d->managed->grabMouse();
    }
    if (d->activeRegion != QDecoration::None &&
         d->activeRegion != QDecoration::Close &&
         d->activeRegion != QDecoration::Minimize &&
         d->activeRegion != QDecoration::Menu) {
        d->managed->raise();
    }

    if (e->button() == Qt::RightButton) {
        menu(e->globalPos());
    }
}

void QWSManager::mouseReleaseEvent(QMouseEvent *e)
{
    Q_D(QWSManager);
    d->managed->releaseMouse();
    if (d->cached_region.regionType && d->previousRegionRepainted && QApplication::mouseButtons() == 0) {
        bool doesHover = repaintRegion(d->cached_region.regionType, QDecoration::Hover);
        if (!doesHover) {
            repaintRegion(d->cached_region.regionType, QDecoration::Normal);
            d->previousRegionRepainted = false;
        }
    }

    if (e->button() == Qt::LeftButton) {
        //handleMove();
        int itm = QApplication::qwsDecoration().regionAt(d->managed, e->globalPos());
        int activatedItem = d->activeRegion;
        d->activeRegion = QDecoration::None;
        d->active = 0;
        if (activatedItem == itm)
            QApplication::qwsDecoration().regionClicked(d->managed, itm);
    } else if (d->activeRegion == QDecoration::None) {
        d->active = 0;
    }
}

void QWSManager::mouseDoubleClickEvent(QMouseEvent *e)
{
    Q_D(QWSManager);
    if (e->button() == Qt::LeftButton)
        QApplication::qwsDecoration().regionDoubleClicked(d->managed,
            QApplication::qwsDecoration().regionAt(d->managed, e->globalPos()));
}

static inline Qt::CursorShape regionToShape(int region)
{
    if (region == QDecoration::None)
        return Qt::ArrowCursor;

    static const struct {
        int region;
        Qt::CursorShape shape;
    } r2s[] = {
        { QDecoration::TopLeft,     Qt::SizeFDiagCursor },
        { QDecoration::Top,         Qt::SizeVerCursor},
        { QDecoration::TopRight,    Qt::SizeBDiagCursor},
        { QDecoration::Left,        Qt::SizeHorCursor},
        { QDecoration::Right,       Qt::SizeHorCursor},
        { QDecoration::BottomLeft,  Qt::SizeBDiagCursor},
        { QDecoration::Bottom,      Qt::SizeVerCursor},
        { QDecoration::BottomRight, Qt::SizeFDiagCursor},
        { QDecoration::None,        Qt::ArrowCursor}
    };

    int i = 0;
    while (region != r2s[i].region && r2s[i].region)
        ++i;
    return r2s[i].shape;
}

void QWSManager::mouseMoveEvent(QMouseEvent *e)
{
    Q_D(QWSManager);
    if (d->newCachedRegion(e->globalPos())) {
        if(d->previousRegionType && d->previousRegionRepainted)
            repaintRegion(d->previousRegionType, QDecoration::Normal);
        if(d->cached_region.regionType) {
            d->previousRegionRepainted = repaintRegion(d->cached_region.regionType, QDecoration::Hover);
        }
    }


#ifndef QT_NO_CURSOR
    if (d->managed->minimumSize() != d->managed->maximumSize()) {
        QWSDisplay *qwsd = QApplication::desktop()->qwsDisplay();
        qwsd->selectCursor(d->managed, regionToShape(d->cachedRegionAt()));
    }
#endif //QT_NO_CURSOR

    if (d->activeRegion)
        handleMove(e->globalPos());
}

void QWSManager::handleMove(QPoint g)
{
    Q_D(QWSManager);

    // don't allow dragging to where the user probably cannot click!
    QApplicationPrivate *ap = QApplicationPrivate::instance();
    const QRect maxWindowRect = ap->maxWindowRect(qt_screen);
    if (maxWindowRect.isValid()) {
        if (g.x() < maxWindowRect.x())
            g.setX(maxWindowRect.x());
        if (g.y() < maxWindowRect.y())
            g.setY(maxWindowRect.y());
        if (g.x() > maxWindowRect.right())
            g.setX(maxWindowRect.right());
        if (g.y() > maxWindowRect.bottom())
            g.setY(maxWindowRect.bottom());
    }

    if (g == d->mousePos)
        return;

    if ( d->managed->isMaximized() )
        return;

    int x = d->managed->geometry().x();
    int y = d->managed->geometry().y();
    int w = d->managed->width();
    int h = d->managed->height();

    QRect geom(d->managed->geometry());

    QPoint delta = g - d->mousePos;
    d->mousePos = g;

    if (d->activeRegion == QDecoration::Title) {
        geom = QRect(x + delta.x(), y + delta.y(), w, h);
    } else {
        bool keepTop = true;
        bool keepLeft = true;
        switch (d->activeRegion) {
        case QDecoration::Top:
            geom.setTop(geom.top() + delta.y());
            keepTop = false;
            break;
        case QDecoration::Bottom:
            geom.setBottom(geom.bottom() + delta.y());
            keepTop = true;
            break;
        case QDecoration::Left:
            geom.setLeft(geom.left() + delta.x());
            keepLeft = false;
            break;
        case QDecoration::Right:
            geom.setRight(geom.right() + delta.x());
            keepLeft = true;
            break;
        case QDecoration::TopRight:
            geom.setTopRight(geom.topRight() + delta);
            keepLeft = true;
            keepTop = false;
            break;
        case QDecoration::TopLeft:
            geom.setTopLeft(geom.topLeft() + delta);
            keepLeft = false;
            keepTop = false;
            break;
        case QDecoration::BottomLeft:
            geom.setBottomLeft(geom.bottomLeft() + delta);
            keepLeft = false;
            keepTop = true;
            break;
        case QDecoration::BottomRight:
            geom.setBottomRight(geom.bottomRight() + delta);
            keepLeft = true;
            keepTop = true;
            break;
        default:
            return;
        }

        QSize newSize = QLayout::closestAcceptableSize(d->managed, geom.size());

        int dx = newSize.width() - geom.width();
        int dy = newSize.height() - geom.height();

        if (keepTop) {
            geom.setBottom(geom.bottom() + dy);
            d->mousePos.ry() += dy;
        } else {
            geom.setTop(geom.top() - dy);
            d->mousePos.ry() -= dy;
        }
        if (keepLeft) {
            geom.setRight(geom.right() + dx);
            d->mousePos.rx() += dx;
        } else {
            geom.setLeft(geom.left() - dx);
            d->mousePos.rx() -= dx;
        }
    }
    if (geom != d->managed->geometry()) {
        QApplication::sendPostedEvents();
        d->managed->setGeometry(geom);
    }
}

void QWSManager::paintEvent(QPaintEvent *)
{
     Q_D(QWSManager);
     d->dirtyRegion(QDecoration::All, QDecoration::Normal);
}

void QWSManagerPrivate::dirtyRegion(int decorationRegion,
                                    QDecoration::DecorationState state,
                                    const QRegion &clip)
{
    QTLWExtra *topextra = managed->d_func()->extra->topextra;
    QWidgetBackingStore *bs = topextra->backingStore.data();
    const bool pendingUpdateRequest = bs->isDirty();

    if (decorationRegion == QDecoration::All) {
        if (clip.isEmpty())
            entireDecorationNeedsRepaint = true;
        dirtyRegions.clear();
        dirtyStates.clear();
    }
    int i = dirtyRegions.indexOf(decorationRegion);
    if (i >= 0) {
        dirtyRegions.removeAt(i);
        dirtyStates.removeAt(i);
    }

    dirtyRegions.append(decorationRegion);
    dirtyStates.append(state);
    if (!entireDecorationNeedsRepaint)
        dirtyClip += clip;

    if (!pendingUpdateRequest)
        QApplication::postEvent(managed, new QEvent(QEvent::UpdateRequest), Qt::LowEventPriority);
}

void QWSManagerPrivate::clearDirtyRegions()
{
    dirtyRegions.clear();
    dirtyStates.clear();
    dirtyClip = QRegion();
    entireDecorationNeedsRepaint = false;
}

bool QWSManager::repaintRegion(int decorationRegion, QDecoration::DecorationState state)
{
    Q_D(QWSManager);

    d->dirtyRegion(decorationRegion, state);
    return true;
}

void QWSManager::menu(const QPoint &pos)
{
#ifdef QT_NO_MENU
    Q_UNUSED(pos);
#else
    Q_D(QWSManager);
    if (d->popup)
        delete d->popup;

    // Basic window operation menu
    d->popup = new QMenu();
    QApplication::qwsDecoration().buildSysMenu(d->managed, d->popup);
    connect(d->popup, SIGNAL(triggered(QAction*)), SLOT(menuTriggered(QAction*)));

    d->popup->popup(pos);
    d->activeRegion = QDecoration::None;
#endif // QT_NO_MENU
}

void QWSManager::menuTriggered(QAction *action)
{
#ifdef QT_NO_MENU
    Q_UNUSED(action);
#else
    Q_D(QWSManager);
    QApplication::qwsDecoration().menuTriggered(d->managed, action);
    d->popup->deleteLater();
    d->popup = 0;
#endif
}

void QWSManager::startMove()
{
    Q_D(QWSManager);
    d->mousePos = QCursor::pos();
    d->activeRegion = QDecoration::Title;
    d->active = d->managed;
    d->managed->grabMouse();
}

void QWSManager::startResize()
{
    Q_D(QWSManager);
    d->activeRegion = QDecoration::BottomRight;
    d->active = d->managed;
    d->managed->grabMouse();
}

void QWSManager::maximize()
{
    Q_D(QWSManager);
    // find out how much space the decoration needs
    const int screen = QApplication::desktop()->screenNumber(d->managed);
    const QRect desk = QApplication::desktop()->availableGeometry(screen);
    QRect dummy(0, 0, 1, 1);
    QRect nr;
    QRegion r = QApplication::qwsDecoration().region(d->managed, dummy);
    if (r.isEmpty()) {
        nr = desk;
    } else {
        r += dummy; // make sure we get the full window region in case of 0 width borders
        QRect rect = r.boundingRect();
        nr = QRect(desk.x()-rect.x(), desk.y()-rect.y(),
                desk.width() - (rect.width()==1 ? 0 : rect.width()-1), // ==1 -> dummy
                desk.height() - (rect.height()==1 ? 0 : rect.height()-1));
    }
    d->managed->setGeometry(nr);
}

bool QWSManagerPrivate::newCachedRegion(const QPoint &pos)
{
    // Check if anything has changed that would affect the region caching
    if (managed->windowFlags() == cached_region.windowFlags
        && managed->geometry() == cached_region.windowGeometry
        && cached_region.region.contains(pos))
        return false;

    // Update the cached region
    int reg = QApplication::qwsDecoration().regionAt(managed, pos);
    if (QWidget::mouseGrabber())
        reg = QDecoration::None;

    previousRegionType = cached_region.regionType;
    cached_region.regionType = reg;
    cached_region.region = QApplication::qwsDecoration().region(managed, managed->geometry(),
                                                                reg);
    // Make room for borders around the widget, even if the decoration doesn't have a frame.
    if (reg && !(reg & int(QDecoration::Borders))) {
        cached_region.region -= QApplication::qwsDecoration().region(managed, managed->geometry(), QDecoration::Borders);
    }
    cached_region.windowFlags = managed->windowFlags();
    cached_region.windowGeometry = managed->geometry();
//    QRect rec = cached_region.region.boundingRect();
//    qDebug("Updated cached region: 0x%04x (%d, %d)  (%d, %d,  %d, %d)",
//           reg, pos.x(), pos.y(), rec.x(), rec.y(), rec.right(), rec.bottom());
    return true;
}

QT_END_NAMESPACE

#endif //QT_NO_QWS_MANAGER
