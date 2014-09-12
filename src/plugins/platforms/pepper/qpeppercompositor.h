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

#ifndef QPEPPERCOMPOSITOR_H
#define QPEPPERCOMPOSITOR_H

#include <QtGui>
#include <QtCore>
#include "qpepperinstance.h"

#include "ppapi/cpp/graphics_2d.h"
#include "ppapi/cpp/image_data.h"
#include <ppapi/utility/completion_callback_factory.h>

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

Q_DECLARE_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_COMPOSITOR)

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
    void beginResize(QSize newSize, qreal newDevicePixelRatio);          // call when the frame buffer geometry changes
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

#endif
