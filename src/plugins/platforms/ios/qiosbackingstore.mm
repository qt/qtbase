/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qiosbackingstore.h"
#include "qioswindow.h"

#include <QtOpenGL/private/qgl_p.h>
#include <QtOpenGL/private/qglpaintdevice_p.h>

#include <QtDebug>

class EAGLPaintDevice;

@interface PaintDeviceHelper : NSObject {
    EAGLPaintDevice *device;
}

@property (nonatomic, assign) EAGLPaintDevice *device;

- (void)eaglView:(EAGLView *)view usesFramebuffer:(GLuint)buffer;

@end

class EAGLPaintDevice : public QGLPaintDevice
{
public:
    EAGLPaintDevice(QWindow *window)
        :QGLPaintDevice(), mWindow(window)
    {
#if defined(QT_OPENGL_ES_2)
        helper = [[PaintDeviceHelper alloc] init];
        helper.device = this;
        EAGLView *view = static_cast<QIOSWindow *>(window->handle())->nativeView();
        view.delegate = helper;
        m_thisFBO = view.fbo;
#endif
    }

    ~EAGLPaintDevice()
    {
#if defined(QT_OPENGL_ES_2)
        [helper release];
#endif
    }

    void setFramebuffer(GLuint buffer) { m_thisFBO = buffer; }
    int devType() const { return QInternal::OpenGL; }
    QSize size() const { return mWindow->geometry().size(); }
    QGLContext* context() const {
        // Todo: siplify this:
        return QGLContext::fromOpenGLContext(
            static_cast<QIOSWindow *>(mWindow->handle())->glContext()->context());
    }

    QPaintEngine *paintEngine() const { return qt_qgl_paint_engine(); }

private:
    QWindow *mWindow;
    PaintDeviceHelper *helper;
};

@implementation PaintDeviceHelper
@synthesize device;

- (void)eaglView:(EAGLView *)view usesFramebuffer:(GLuint)buffer
{
    Q_UNUSED(view)
    if (device)
        device->setFramebuffer(buffer);
}

@end

QT_BEGIN_NAMESPACE

QIOSBackingStore::QIOSBackingStore(QWindow *window)
    : QPlatformBackingStore(window), mPaintDevice(new EAGLPaintDevice(window))
{
}

QPaintDevice *QIOSBackingStore::paintDevice()
{
    return mPaintDevice;
}

void QIOSBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(region);
    Q_UNUSED(offset);
    qDebug() << __FUNCTION__ << "not implemented";
    //static_cast<QIOSWindow *>(window->handle())->glContext()->swapBuffers();
}

void QIOSBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(size);
    Q_UNUSED(staticContents);
    qDebug() << __FUNCTION__ << "not implemented";
}

QT_END_NAMESPACE
