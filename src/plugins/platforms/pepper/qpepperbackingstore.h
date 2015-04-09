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

#ifndef QPEPPERBACKINGSTORE_H
#define QPEPPERBACKINGSTORE_H

#include <QtCore/qloggingcategory.h>
#include <qpa/qplatformbackingstore.h>

#include <ppapi/cpp/graphics_2d.h>
#include <ppapi/cpp/image_data.h>
#include <ppapi/utility/completion_callback_factory.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_BACKINGSTORE)

class QPepperWindow;
class QPepperCompositor;
class QPepperInstance;
class QPepperBackingStore : public QPlatformBackingStore
{
public:
    QPepperBackingStore(QWindow *window);
    ~QPepperBackingStore();

    QPaintDevice *paintDevice() Q_DECL_OVERRIDE;
    void flush(QWindow *widget, const QRegion &region, const QPoint &offset) Q_DECL_OVERRIDE;
    void resize(const QSize &size, const QRegion &) Q_DECL_OVERRIDE;
    void beginPaint(const QRegion &) Q_DECL_OVERRIDE;
    void endPaint() Q_DECL_OVERRIDE;

    void createFrameBuffer(QSize size, qreal devicePixelRatio);
    void setFrameBuffer(QImage *frameBuffer);
    void setPepperInstance(QPepperInstance *instance);
    void flushCompletedCallback(int32_t);

private:
    bool m_isInPaint;
    bool m_isInFlush;
    QSize m_size;
    QPepperCompositor *m_compositor;
    pp::Graphics2D *m_context2D;
    pp::ImageData *m_imageData2D;
    QImage *m_frameBuffer;
    bool m_ownsFrameBuffer;
    pp::CompletionCallbackFactory<QPepperBackingStore> m_callbackFactory;
};

QT_END_NAMESPACE

#endif
