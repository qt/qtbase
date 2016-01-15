/***************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
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

#ifndef QQNXRASTERWINDOW_H
#define QQNXRASTERWINDOW_H

#include "qqnxwindow.h"
#include "qqnxbuffer.h"

QT_BEGIN_NAMESPACE

class QQnxRasterWindow : public QQnxWindow
{
public:
    QQnxRasterWindow(QWindow *window, screen_context_t context, bool needRootWindow);

    void post(const QRegion &dirty);

    void scroll(const QRegion &region, int dx, int dy, bool flush=false);

    QQnxBuffer &renderBuffer();

    bool hasBuffers() const { return !bufferSize().isEmpty(); }

    void setParent(const QPlatformWindow *window);

    void adjustBufferSize();

protected:
    int pixelFormat() const;
    void resetBuffers();

    // Copies content from the previous buffer (back buffer) to the current buffer (front buffer)
    void blitPreviousToCurrent(const QRegion &region, int dx, int dy, bool flush=false);

    void blitHelper(QQnxBuffer &source, QQnxBuffer &target, const QPoint &sourceOffset,
                    const QPoint &targetOffset, const QRegion &region, bool flush = false);

private:
    QRegion m_previousDirty;
    QRegion m_scrolled;
    int m_currentBufferIndex;
    int m_previousBufferIndex;
    QQnxBuffer m_buffers[MAX_BUFFER_COUNT];
};

QT_END_NAMESPACE

#endif // QQNXRASTERWINDOW_H
