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

#ifndef QEGLFSKMSCURSOR_H
#define QEGLFSKMSCURSOR_H

#include <qpa/qplatformcursor.h>
#include <QtCore/QList>
#include <QtGui/QImage>
#include <QtGui/private/qinputdevicemanager_p.h>

#include <gbm.h>

QT_BEGIN_NAMESPACE

class QEglFSKmsScreen;
class QEglFSKmsCursor;

class QEglFSKmsCursorDeviceListener : public QObject
{
    Q_OBJECT

public:
    QEglFSKmsCursorDeviceListener(QEglFSKmsCursor *cursor) : m_cursor(cursor) { }
    bool hasMouse() const;

public slots:
    void onDeviceListChanged(QInputDeviceManager::DeviceType type);

private:
    QEglFSKmsCursor *m_cursor;
};

class QEglFSKmsCursor : public QPlatformCursor
{
    Q_OBJECT

public:
    QEglFSKmsCursor(QEglFSKmsScreen *screen);
    ~QEglFSKmsCursor();

    // input methods
    void pointerEvent(const QMouseEvent & event) Q_DECL_OVERRIDE;
#ifndef QT_NO_CURSOR
    void changeCursor(QCursor * windowCursor, QWindow * window) Q_DECL_OVERRIDE;
#endif
    QPoint pos() const Q_DECL_OVERRIDE;
    void setPos(const QPoint &pos) Q_DECL_OVERRIDE;

    void updateMouseStatus();

private:
    void initCursorAtlas();

    enum CursorState {
        CursorDisabled,
        CursorPendingHidden,
        CursorHidden,
        CursorPendingVisible,
        CursorVisible
    };

    QEglFSKmsScreen *m_screen;
    QSize m_cursorSize;
    gbm_bo *m_bo;
    QPoint m_pos;
    QPlatformCursorImage m_cursorImage;
    CursorState m_state;
    QEglFSKmsCursorDeviceListener *m_deviceListener;

    // cursor atlas information
    struct CursorAtlas {
        CursorAtlas() : cursorsPerRow(0), cursorWidth(0), cursorHeight(0) { }
        int cursorsPerRow;
        int width, height; // width and height of the atlas
        int cursorWidth, cursorHeight; // width and height of cursors inside the atlas
        QList<QPoint> hotSpots;
        QImage image;
    } m_cursorAtlas;
};

QT_END_NAMESPACE

#endif
