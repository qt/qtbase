/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
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
      mCursorImage(nullptr),
      mDeviceListener(nullptr)
{
    const char *envVar = "QT_QPA_FB_HIDECURSOR";
    if (qEnvironmentVariableIsSet(envVar))
        mVisible = qEnvironmentVariableIntValue(envVar) == 0;
    if (!mVisible)
        return;

    mCursorImage = new QPlatformCursorImage(0, 0, 0, 0, 0, 0);
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

QRect QFbCursor::getCurrentRect() const
{
    QRect rect = mCursorImage->image()->rect().translated(-mCursorImage->hotspot().x(),
                                                          -mCursorImage->hotspot().y());
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
    if (!mVisible)
        return;
    mCurrentRect = getCurrentRect();
    if (mOnScreen || mScreen->geometry().intersects(mCurrentRect.translated(mScreen->geometry().topLeft())))
        setDirty();
}

void QFbCursor::pointerEvent(const QMouseEvent &e)
{
    if (e.type() != QEvent::MouseMove)
        return;
    m_pos = e.screenPos().toPoint();
    if (!mVisible)
        return;
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
    painter.drawImage(mPrevRect, *mCursorImage->image());
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
    if (mCursorImage)
        mCursorImage->set(shape);
}

void QFbCursor::setCursor(const QImage &image, int hotx, int hoty)
{
    if (mCursorImage)
        mCursorImage->set(image, hotx, hoty);
}

void QFbCursor::setCursor(const uchar *data, const uchar *mask, int width, int height, int hotX, int hotY)
{
    if (mCursorImage)
        mCursorImage->set(data, mask, width, height, hotX, hotY);
}

#ifndef QT_NO_CURSOR
void QFbCursor::changeCursor(QCursor * widgetCursor, QWindow *window)
{
    Q_UNUSED(window);
    if (!mVisible)
        return;
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
    mVisible = mDeviceListener ? mDeviceListener->hasMouse() : false;
    mScreen->setDirty(mVisible ? getCurrentRect() : lastPainted());
}

QT_END_NAMESPACE
