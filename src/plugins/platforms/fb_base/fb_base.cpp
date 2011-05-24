/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "fb_base.h"
#include <qpainter.h>
#include <qdebug.h>
#include <qbitmap.h>
#include <QPlatformCursor>
#include <QWindowSystemInterface>

QPlatformSoftwareCursor::QPlatformSoftwareCursor(QPlatformScreen *scr)
        : QPlatformCursor(scr), currentRect(QRect()), prevRect(QRect())
{
    graphic = new QPlatformCursorImage(0, 0, 0, 0, 0, 0);
    setCursor(Qt::ArrowCursor);
}

QRect QPlatformSoftwareCursor::getCurrentRect()
{
    QRect rect = graphic->image()->rect().translated(-graphic->hotspot().x(),
                                                     -graphic->hotspot().y());
    rect.translate(QCursor::pos());
    QPoint screenOffset = screen->geometry().topLeft();
    rect.translate(-screenOffset);  // global to local translation
    return rect;
}


void QPlatformSoftwareCursor::pointerEvent(const QMouseEvent & e)
{
    Q_UNUSED(e);
    QPoint screenOffset = screen->geometry().topLeft();
    currentRect = getCurrentRect();
    // global to local translation
    if (onScreen || screen->geometry().intersects(currentRect.translated(screenOffset))) {
        setDirty();
    }
}

QRect QPlatformSoftwareCursor::drawCursor(QPainter & painter)
{
    dirty = false;
    if (currentRect.isNull())
        return QRect();

    // We need this because the cursor might be dirty due to moving off screen
    QPoint screenOffset = screen->geometry().topLeft();
    // global to local translation
    if (!currentRect.translated(screenOffset).intersects(screen->geometry()))
        return QRect();

    prevRect = currentRect;
    painter.drawImage(prevRect, *graphic->image());
    onScreen = true;
    return prevRect;
}

QRect QPlatformSoftwareCursor::dirtyRect()
{
    if (onScreen) {
        onScreen = false;
        return prevRect;
    }
    return QRect();
}

void QPlatformSoftwareCursor::setCursor(Qt::CursorShape shape)
{
    graphic->set(shape);
}

void QPlatformSoftwareCursor::setCursor(const QImage &image, int hotx, int hoty)
{
    graphic->set(image, hotx, hoty);
}

void QPlatformSoftwareCursor::setCursor(const uchar *data, const uchar *mask, int width, int height, int hotX, int hotY)
{
    graphic->set(data, mask, width, height, hotX, hotY);
}

void QPlatformSoftwareCursor::changeCursor(QCursor * widgetCursor, QWidget * widget)
{
    Q_UNUSED(widget);
    Qt::CursorShape shape = widgetCursor->shape();

    if (shape == Qt::BitmapCursor) {
        // application supplied cursor
        QPoint spot = widgetCursor->hotSpot();
        setCursor(widgetCursor->pixmap().toImage(), spot.x(), spot.y());
    } else {
        // system cursor
        setCursor(shape);
    }
    currentRect = getCurrentRect();
    QPoint screenOffset = screen->geometry().topLeft(); // global to local translation
    if (onScreen || screen->geometry().intersects(currentRect.translated(screenOffset)))
        setDirty();
}

QFbScreen::QFbScreen() : cursor(0), mGeometry(), mDepth(16), mFormat(QImage::Format_RGB16), mScreenImage(0), compositePainter(0), isUpToDate(false)
{
    mScreenImage = new QImage(mGeometry.size(), mFormat);
    redrawTimer.setSingleShot(true);
    redrawTimer.setInterval(0);
    QObject::connect(&redrawTimer, SIGNAL(timeout()), this, SLOT(doRedraw()));
}

void QFbScreen::setGeometry(QRect rect)
{
    delete mScreenImage;
    mGeometry = rect;
    mScreenImage = new QImage(mGeometry.size(), mFormat);
    delete compositePainter;
    compositePainter = 0;
    invalidateRectCache();
}

void QFbScreen::setDepth(int depth)
{
    mDepth = depth;
}

void QFbScreen::setPhysicalSize(QSize size)
{
    mPhysicalSize = size;
}

void QFbScreen::setFormat(QImage::Format format)
{
    mFormat = format;
    delete mScreenImage;
    mScreenImage = new QImage(mGeometry.size(), mFormat);
    delete compositePainter;
    compositePainter = 0;
}

QFbScreen::~QFbScreen()
{
    delete compositePainter;
    delete mScreenImage;
}

void QFbScreen::setDirty(const QRect &rect)
{
    QRect intersection = rect.intersected(mGeometry);
    QPoint screenOffset = mGeometry.topLeft();
    repaintRegion += intersection.translated(-screenOffset);    // global to local translation
    if (!redrawTimer.isActive()) {
        redrawTimer.start();
    }
}

void QFbScreen::generateRects()
{
    cachedRects.clear();
    QPoint screenOffset = mGeometry.topLeft();
    QRegion remainingScreen(mGeometry.translated(-screenOffset)); // global to local translation

    for (int i = 0; i < windowStack.length(); i++) {
        if (remainingScreen.isEmpty())
            break;
        if (!windowStack[i]->visible())
            continue;
        if (windowStack[i]->widget()->isMinimized())
            continue;

        if (!windowStack[i]->widget()->testAttribute(Qt::WA_TranslucentBackground)) {
            QRect localGeometry = windowStack.at(i)->geometry().translated(-screenOffset); // global to local translation
            remainingScreen -= localGeometry;
            QRegion windowRegion(localGeometry);
            windowRegion -= remainingScreen;
            foreach(QRect rect, windowRegion.rects()) {
                cachedRects += QPair<QRect, int>(rect, i);
            }
        }
    }
    foreach (QRect rect, remainingScreen.rects())
        cachedRects += QPair<QRect, int>(rect, -1);
    isUpToDate = true;
    return;
}



QRegion QFbScreen::doRedraw()
{
    QPoint screenOffset = mGeometry.topLeft();

    QRegion touchedRegion;
    if (cursor && cursor->isDirty() && cursor->isOnScreen()) {
        QRect lastCursor = cursor->dirtyRect();
        repaintRegion += lastCursor;
    }
    if (repaintRegion.isEmpty() && (!cursor || !cursor->isDirty())) {
        return touchedRegion;
    }

    QVector<QRect> rects = repaintRegion.rects();

    if (!isUpToDate)
        generateRects();

    if (!compositePainter)
        compositePainter = new QPainter(mScreenImage);
    for (int rectIndex = 0; rectIndex < repaintRegion.numRects(); rectIndex++) {
        QRegion rectRegion = rects[rectIndex];

        for(int i = 0; i < cachedRects.length(); i++) {
            QRect screenSubRect = cachedRects[i].first;
            int layer = cachedRects[i].second;
            QRegion intersect = rectRegion.intersected(screenSubRect);

            if (intersect.isEmpty())
                continue;

            rectRegion -= intersect;

            // we only expect one rectangle, but defensive coding...
            foreach (QRect rect, intersect.rects()) {
                bool firstLayer = true;
                if (layer == -1) {
                    compositePainter->fillRect(rect, Qt::black);
                    firstLayer = false;
                    layer = windowStack.size() - 1;
                }

                for (int layerIndex = layer; layerIndex != -1; layerIndex--) {
                    if (!windowStack[layerIndex]->visible())
                        continue;
                    if (windowStack[layerIndex]->widget()->isMinimized())
                        continue;
                    QRect windowRect = windowStack[layerIndex]->geometry().translated(-screenOffset);
                    QRect windowIntersect = rect.translated(-windowRect.left(),
                                                            -windowRect.top());
                    compositePainter->drawImage(rect, windowStack[layerIndex]->surface->image(),
                                                windowIntersect);
                    if (firstLayer) {
                        firstLayer = false;
                    }
                }
            }
        }
    }

    QRect cursorRect;
    if (cursor && (cursor->isDirty() || repaintRegion.intersects(cursor->lastPainted()))) {
        cursorRect = cursor->drawCursor(*compositePainter);
        touchedRegion += cursorRect;
    }
    touchedRegion += repaintRegion;
    repaintRegion = QRegion();



//    qDebug() << "QFbScreen::doRedraw"  << windowStack.size() << mScreenImage->size() << touchedRegion;


    return touchedRegion;
}

void QFbScreen::addWindow(QFbWindow *surface)
{
    windowStack.prepend(surface);
    surface->mScreens.append(this);
    invalidateRectCache();
    setDirty(surface->geometry());
}

void QFbScreen::removeWindow(QFbWindow * surface)
{
    windowStack.removeOne(surface);
    surface->mScreens.removeOne(this);
    invalidateRectCache();
    setDirty(surface->geometry());
}

void QFbWindow::raise()
{
    QList<QFbScreen *>::const_iterator i = mScreens.constBegin();
    QList<QFbScreen *>::const_iterator end = mScreens.constEnd();
    while (i != end) {
        (*i)->raise(this);
        ++i;
    }
}

void QFbScreen::raise(QPlatformWindow * surface)
{
    QFbWindow *s = static_cast<QFbWindow *>(surface);
    int index = windowStack.indexOf(s);
    if (index <= 0)
        return;
    windowStack.move(index, 0);
    invalidateRectCache();
    setDirty(s->geometry());
}

void QFbWindow::lower()
{
    QList<QFbScreen *>::const_iterator i = mScreens.constBegin();
    QList<QFbScreen *>::const_iterator end = mScreens.constEnd();
    while (i != end) {
        (*i)->lower(this);
        ++i;
    }
}

void QFbScreen::lower(QPlatformWindow * surface)
{
    QFbWindow *s = static_cast<QFbWindow *>(surface);
    int index = windowStack.indexOf(s);
    if (index == -1 || index == (windowStack.size() - 1))
        return;
    windowStack.move(index, windowStack.size() - 1);
    invalidateRectCache();
    setDirty(s->geometry());
}

QWidget * QFbScreen::topLevelAt(const QPoint & p) const
{
    for(int i = 0; i < windowStack.size(); i++) {
        if (windowStack[i]->geometry().contains(p, false) &&
            windowStack[i]->visible() &&
            !windowStack[i]->widget()->isMinimized()) {
            return windowStack[i]->widget();
        }
    }
    return 0;
}

QFbWindow::QFbWindow(QWidget *window)
    :QPlatformWindow(window),
      visibleFlag(false)
{
    static QAtomicInt winIdGenerator(1);
    windowId = winIdGenerator.fetchAndAddRelaxed(1);
}


QFbWindow::~QFbWindow()
{
    QList<QFbScreen *>::const_iterator i = mScreens.constBegin();
    QList<QFbScreen *>::const_iterator end = mScreens.constEnd();
    while (i != end) {
        (*i)->removeWindow(this);
        ++i;
    }
}


QFbWindowSurface::QFbWindowSurface(QFbScreen *screen, QWidget *window)
    : QWindowSurface(window),
      mScreen(screen)
{
    mImage = QImage(window->size(), mScreen->format());

    platformWindow = static_cast<QFbWindow*>(window->platformWindow());
    platformWindow->surface = this;
}

QFbWindowSurface::~QFbWindowSurface()
{
}

void QFbWindowSurface::flush(QWidget *widget, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(widget);
    Q_UNUSED(offset);


//    qDebug() << "QFbWindowSurface::flush" << region;


    platformWindow->repaint(region);
}


void QFbWindow::repaint(const QRegion &region)
{
    QRect currentGeometry = geometry();

    QRect dirtyClient = region.boundingRect();
    QRect dirtyRegion(currentGeometry.left() + dirtyClient.left(),
                      currentGeometry.top() + dirtyClient.top(),
                      dirtyClient.width(),
                      dirtyClient.height());
    QList<QFbScreen *>::const_iterator i = mScreens.constBegin();
    QList<QFbScreen *>::const_iterator end = mScreens.constEnd();
    QRect oldGeometryLocal = oldGeometry;
    oldGeometry = currentGeometry;
    while (i != end) {
        // If this is a move, redraw the previous location
        if (oldGeometryLocal != currentGeometry) {
            (*i)->setDirty(oldGeometryLocal);
        }
        (*i)->setDirty(dirtyRegion);
        ++i;
    }
}

void QFbWindowSurface::resize(const QSize &size)
{
    // change the widget's QImage if this is a resize
    if (mImage.size() != size)
        mImage = QImage(size, mScreen->format());
    QWindowSurface::resize(size);
}

void QFbWindow::setGeometry(const QRect &rect)
{
// store previous geometry for screen update
    oldGeometry = geometry();


    QList<QFbScreen *>::const_iterator i = mScreens.constBegin();
    QList<QFbScreen *>::const_iterator end = mScreens.constEnd();
    while (i != end) {
        (*i)->invalidateRectCache();
        ++i;
    }
//###    QWindowSystemInterface::handleGeometryChange(window(), rect);

    QPlatformWindow::setGeometry(rect);
}

bool QFbWindowSurface::scroll(const QRegion &area, int dx, int dy)
{
    return QWindowSurface::scroll(area, dx, dy);
}

void QFbWindowSurface::beginPaint(const QRegion &region)
{
    Q_UNUSED(region);
}

void QFbWindowSurface::endPaint(const QRegion &region)
{
    Q_UNUSED(region);
}

void QFbWindow::setVisible(bool visible)
{
    visibleFlag = visible;
    QList<QFbScreen *>::const_iterator i = mScreens.constBegin();
    QList<QFbScreen *>::const_iterator end = mScreens.constEnd();
    while (i != end) {
        (*i)->invalidateRectCache();
        (*i)->setDirty(geometry());
        ++i;
    }
}

Qt::WindowFlags QFbWindow::setWindowFlags(Qt::WindowFlags type)
{
    flags = type;
    QList<QFbScreen *>::const_iterator i = mScreens.constBegin();
    QList<QFbScreen *>::const_iterator end = mScreens.constEnd();
    while (i != end) {
        (*i)->invalidateRectCache();
        ++i;
    }
    return flags;
}

Qt::WindowFlags QFbWindow::windowFlags() const
{
    return flags;
}
