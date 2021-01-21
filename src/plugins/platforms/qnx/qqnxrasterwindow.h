/***************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
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

    void setParent(const QPlatformWindow *window) override;

    void adjustBufferSize();

protected:
    int pixelFormat() const override;
    void resetBuffers() override;

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
