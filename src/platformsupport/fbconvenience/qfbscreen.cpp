/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qfbscreen_p.h"
#include "qfbcursor_p.h"
#include "qfbwindow_p.h"
#include "qfbbackingstore_p.h"

#include <QtGui/QPainter>

QT_BEGIN_NAMESPACE

QFbScreen::QFbScreen() : cursor(0), mGeometry(), mDepth(16), mFormat(QImage::Format_RGB16), mScreenImage(0), compositePainter(0), isUpToDate(false)
{
    mScreenImage = new QImage(mGeometry.size(), mFormat);
    redrawTimer.setSingleShot(true);
    redrawTimer.setInterval(0);
    connect(&redrawTimer, SIGNAL(timeout()), this, SLOT(doRedraw()));
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

QWindow *QFbScreen::topLevelAt(const QPoint & p) const
{
    Q_UNUSED(p);
#if 0
    for (int i = 0; i < windowStack.size(); i++) {
        if (windowStack[i]->geometry().contains(p, false) &&
            windowStack[i]->visible() &&
            !windowStack[i]->widget()->isMinimized()) {
            return windowStack[i]->widget();
        }
    }
#endif
    return 0;
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
#if 0
        if (!windowStack[i]->isVisible())
            continue;
        if (windowStack[i]->isMinimized())
            continue;

        if (!windowStack[i]->testAttribute(Qt::WA_TranslucentBackground)) {
            QRect localGeometry = windowStack.at(i)->geometry().translated(-screenOffset); // global to local translation
            remainingScreen -= localGeometry;
            QRegion windowRegion(localGeometry);
            windowRegion -= remainingScreen;
            foreach (QRect rect, windowRegion.rects()) {
                cachedRects += QPair<QRect, int>(rect, i);
            }
        }
#endif
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
    for (int rectIndex = 0; rectIndex < repaintRegion.rectCount(); rectIndex++) {
        QRegion rectRegion = rects[rectIndex];

        for (int i = 0; i < cachedRects.length(); i++) {
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
                    if (!windowStack[layerIndex]->isVisible())
                        continue;
                    // if (windowStack[layerIndex]->isMinimized())
                    //     continue;
                    QRect windowRect = windowStack[layerIndex]->geometry().translated(-screenOffset);
                    QRect windowIntersect = rect.translated(-windowRect.left(),
                                                            -windowRect.top());
                    compositePainter->drawImage(rect, windowStack[layerIndex]->backingStore()->image(),
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

QT_END_NAMESPACE

