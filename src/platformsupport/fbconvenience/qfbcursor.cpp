/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qfbcursor_p.h"
#include "qfbscreen_p.h"
#include <QtGui/QPainter>
#include <QtGui/private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

bool QFbCursorDeviceListener::hasMouse() const
{
    return QGuiApplicationPrivate::inputDeviceManager()->deviceCount(QInputDeviceManager::DeviceTypePointer) > 0;
}

void QFbCursorDeviceListener::onDeviceListChanged(QInputDeviceManager::DeviceType type)
{
    if (type == QInputDeviceManager::DeviceTypePointer)
        m_cursor->updateMouseStatus();
}

QFbCursor::QFbCursor(QFbScreen *screen)
    : mVisible(true),
      mScreen(screen),
      mDirty(false),
      mOnScreen(false),
      mGraphic(0),
      mDeviceListener(0)
{
    QByteArray hideCursorVal = qgetenv("QT_QPA_FB_HIDECURSOR");
    if (!hideCursorVal.isEmpty())
        mVisible = hideCursorVal.toInt() == 0;
    if (!mVisible)
        return;

    mGraphic = new QPlatformCursorImage(0, 0, 0, 0, 0, 0);
    setCursor(Qt::ArrowCursor);

    mDeviceListener = new QFbCursorDeviceListener(this);
    connect(QGuiApplicationPrivate::inputDeviceManager(), &QInputDeviceManager::deviceListChanged,
            mDeviceListener, &QFbCursorDeviceListener::onDeviceListChanged);
    updateMouseStatus();
}

QFbCursor::~QFbCursor()
{
    delete mDeviceListener;
}

QRect QFbCursor::getCurrentRect()
{
    QRect rect = mGraphic->image()->rect().translated(-mGraphic->hotspot().x(),
                                                     -mGraphic->hotspot().y());
    rect.translate(m_pos);
    QPoint mScreenOffset = mScreen->geometry().topLeft();
    rect.translate(-mScreenOffset);  // global to local translation
    return rect;
}

QPoint QFbCursor::pos() const
{
    return m_pos;
}

void QFbCursor::setPos(const QPoint &pos)
{
    QGuiApplicationPrivate::inputDeviceManager()->setCursorPos(pos);
    m_pos = pos;
    mCurrentRect = getCurrentRect();
    if (mOnScreen || mScreen->geometry().intersects(mCurrentRect.translated(mScreen->geometry().topLeft())))
        setDirty();
}

void QFbCursor::pointerEvent(const QMouseEvent &e)
{
    if (e.type() != QEvent::MouseMove)
        return;
    m_pos = e.screenPos().toPoint();
    mCurrentRect = getCurrentRect();
    if (mOnScreen || mScreen->geometry().intersects(mCurrentRect.translated(mScreen->geometry().topLeft())))
        setDirty();
}

QRect QFbCursor::drawCursor(QPainter & painter)
{
    if (!mVisible)
        return QRect();

    mDirty = false;
    if (mCurrentRect.isNull())
        return QRect();

    // We need this because the cursor might be mDirty due to moving off mScreen
    QPoint mScreenOffset = mScreen->geometry().topLeft();
    // global to local translation
    if (!mCurrentRect.translated(mScreenOffset).intersects(mScreen->geometry()))
        return QRect();

    mPrevRect = mCurrentRect;
    painter.drawImage(mPrevRect, *mGraphic->image());
    mOnScreen = true;
    return mPrevRect;
}

QRect QFbCursor::dirtyRect()
{
    if (mOnScreen) {
        mOnScreen = false;
        return mPrevRect;
    }
    return QRect();
}

void QFbCursor::setCursor(Qt::CursorShape shape)
{
    mGraphic->set(shape);
}

void QFbCursor::setCursor(const QImage &image, int hotx, int hoty)
{
    mGraphic->set(image, hotx, hoty);
}

void QFbCursor::setCursor(const uchar *data, const uchar *mask, int width, int height, int hotX, int hotY)
{
    mGraphic->set(data, mask, width, height, hotX, hotY);
}

#ifndef QT_NO_CURSOR
void QFbCursor::changeCursor(QCursor * widgetCursor, QWindow *window)
{
    Q_UNUSED(window);
    const Qt::CursorShape shape = widgetCursor ? widgetCursor->shape() : Qt::ArrowCursor;

    if (shape == Qt::BitmapCursor) {
        // application supplied cursor
        QPoint spot = widgetCursor->hotSpot();
        setCursor(widgetCursor->pixmap().toImage(), spot.x(), spot.y());
    } else {
        // system cursor
        setCursor(shape);
    }
    mCurrentRect = getCurrentRect();
    QPoint mScreenOffset = mScreen->geometry().topLeft(); // global to local translation
    if (mOnScreen || mScreen->geometry().intersects(mCurrentRect.translated(mScreenOffset)))
        setDirty();
}
#endif

void QFbCursor::setDirty()
{
    if (!mVisible)
        return;

    if (!mDirty) {
        mDirty = true;
        mScreen->scheduleUpdate();
    }
}

void QFbCursor::updateMouseStatus()
{
    mVisible = mDeviceListener->hasMouse();
    mScreen->setDirty(mVisible ? getCurrentRect() : lastPainted());
}

QT_END_NAMESPACE
