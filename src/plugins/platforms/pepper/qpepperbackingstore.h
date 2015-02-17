/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Native Client platform plugin.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#ifndef QPEPPERBACKINGSTORE_H
#define QPEPPERBACKINGSTORE_H

#include <qpa/qplatformbackingstore.h>
#include <QtCore/qloggingcategory.h>

#include <ppapi/cpp/graphics_2d.h>
#include <ppapi/cpp/image_data.h>
#include <ppapi/utility/completion_callback_factory.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_BACKINGSTORE)

class QPepperPlatformWindow;
class QPepperCompositor;
class QPepperInstance;
class QPepperBackingStore : public QPlatformBackingStore
{
public:
    QPepperBackingStore(QWindow *window);
    ~QPepperBackingStore();

    QPaintDevice *paintDevice();
    void beginPaint(const QRegion &);
    void endPaint();
    void flush(QWindow *widget, const QRegion &region, const QPoint &offset);
    void resize(const QSize &size, const QRegion &);

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
