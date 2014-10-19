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

#ifndef QPEPPERBACKINGSTORE_H
#define QPEPPERBACKINGSTORE_H

#include "qpepperplatformwindow.h"
#include <qpa/qplatformbackingstore.h>

#include <ppapi/cpp/graphics_2d.h>
#include <ppapi/cpp/image_data.h>
#include <ppapi/utility/completion_callback_factory.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_BACKINGSTORE)

class QPepperBackingStore : public QPlatformBackingStore
{
public:
    QPepperBackingStore(QWindow *window);
    ~QPepperBackingStore();

    QPaintDevice *paintDevice();
    void beginPaint(const QRegion &);
    void endPaint();
    void flush(QWindow *widget, const QRegion &region, const QPoint &offset);
    void resize (const QSize &size, const QRegion &);

    void createFrameBuffer(QSize size, qreal devicePixelRatio);
    void setFrameBuffer(QImage *frameBuffer);
    void setPepperInstance(QPepperInstance *instance);

    void flushCompletedCallback(int32_t);

    bool m_isInPaint;
    bool m_isInFlush;
private:
    QSize m_size;
    QPepperPlatformWindow *m_PepperWindow;
    QPepperCompositor *m_compositor;
    pp::Graphics2D *m_context2D;
    pp::ImageData *m_imageData2D;
    QImage *m_frameBuffer;
    bool m_ownsFrameBuffer;
    pp::CompletionCallbackFactory<QPepperBackingStore> m_callbackFactory;
};

QT_END_NAMESPACE

#endif
