/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt for Native Client platform plugin.
**
** $QT_BEGIN_LICENSE:LGPL3$
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
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPEPPERCOMPOSITOR_H
#define QPEPPERCOMPOSITOR_H

#include "qpepperinstance_p.h"

#include <QtGui/QRegion>

#include <ppapi/cpp/graphics_2d.h>
#include <ppapi/cpp/image_data.h>
#include <ppapi/utility/completion_callback_factory.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_COMPOSITOR)

class QPepperCompositedWindow
{
public:
    QPepperCompositedWindow();

    QWindow *window;
    QImage *frameBuffer;
    QWindow *parentWindow;
    QRegion damage;
    bool flushPending;
    bool visible;
    QList<QWindow *> childWindows;
};

class QPepperCompositor : public QObject
{
    Q_OBJECT
public:
    QPepperCompositor();
    ~QPepperCompositor();

    // Client API
    void addRasterWindow(QWindow *window, QWindow *parentWindow = 0);
    void removeWindow(QWindow *window);

    void setVisible(QWindow *window, bool visible);
    void raise(QWindow *window);
    void lower(QWindow *window);
    void setParent(QWindow *window, QWindow *parent);

    void setFrameBuffer(QWindow *window, QImage *frameBuffer);
    void flush(QWindow *surface, const QRegion &region);
    void waitForFlushed(QWindow *surface);

    // Server API
    void beginResize(QSize newSize,
                     qreal newDevicePixelRatio); // call when the frame buffer geometry changes
    void endResize();

    // Misc API
public:
    QWindow *windowAt(QPoint p);
    QWindow *keyWindow();
    void maybeComposit();
    void composit();

private:
    void createFrameBuffer();
    void flush2(const QRegion &region);
    void flushCompletedCallback(int32_t);

    QHash<QWindow *, QPepperCompositedWindow> m_compositedWindows;
    QList<QWindow *> m_windowStack;
    QImage *m_frameBuffer;
    pp::Graphics2D *m_context2D;
    pp::ImageData *m_imageData2D;
    QRegion globalDamage; // damage caused by expose, window close, etc.
    bool m_needComposit;
    bool m_inFlush;
    bool m_inResize;
    QSize m_targetSize;
    qreal m_targetDevicePixelRatio;

    pp::CompletionCallbackFactory<QPepperCompositor> m_callbackFactory;
};

QT_END_NAMESPACE

#endif
