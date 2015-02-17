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

#ifndef QPEPPERCOMPOSITOR_H
#define QPEPPERCOMPOSITOR_H

#include "qpepperinstance_p.h"

#include <QtGui>
#include <QtCore>

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

#endif
