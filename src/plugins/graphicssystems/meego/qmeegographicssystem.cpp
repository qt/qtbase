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

#include <QDebug>
#include <private/qpixmap_raster_p.h>
#include <private/qwindowsurface_gl_p.h>
#include <private/qwindowsurface_raster_p.h>
#include <private/qegl_p.h>
#include <private/qglextensions_p.h>
#include <private/qgl_p.h>
#include <private/qimagepixmapcleanuphooks_p.h>
#include <private/qapplication_p.h>
#include <private/qgraphicssystem_runtime_p.h>
#include <private/qimage_p.h>
#include <private/qeglproperties_p.h>
#include <private/qeglcontext_p.h>
#include <private/qpixmap_x11_p.h>

#include "qmeegopixmapdata.h"
#include "qmeegolivepixmapdata.h"
#include "qmeegographicssystem.h"
#include "qmeegoextensions.h"

#include <QTimer>

bool QMeeGoGraphicsSystem::surfaceWasCreated = false;

QHash <Qt::HANDLE, QPixmap*> QMeeGoGraphicsSystem::liveTexturePixmaps;

QList<QMeeGoSwitchCallback> QMeeGoGraphicsSystem::switchCallbacks;

QMeeGoGraphicsSystem::SwitchPolicy QMeeGoGraphicsSystem::switchPolicy = QMeeGoGraphicsSystem::AutomaticSwitch;

QMeeGoGraphicsSystem::QMeeGoGraphicsSystem()
{
    qDebug("Using the meego graphics system");
}

QMeeGoGraphicsSystem::~QMeeGoGraphicsSystem()
{
    qDebug("Meego graphics system destroyed");
    qt_destroy_gl_share_widget();
}

class QMeeGoGraphicsSystemSwitchHandler : public QObject
{
    Q_OBJECT
public:
    QMeeGoGraphicsSystemSwitchHandler();

    void addWidget(QWidget *widget);
    bool eventFilter(QObject *, QEvent *);

    void handleMapNotify();

private slots:
    void removeWidget(QObject *object);
    void switchToRaster();
    void switchToMeeGo();

private:
    int visibleWidgets() const;

private:
    QList<QWidget *> m_widgets;
};

typedef bool(*QX11FilterFunction)(XEvent *event);
Q_GUI_EXPORT void qt_installX11EventFilter(QX11FilterFunction func);

static bool x11EventFilter(XEvent *event);

QMeeGoGraphicsSystemSwitchHandler::QMeeGoGraphicsSystemSwitchHandler()
{
    qt_installX11EventFilter(x11EventFilter);
}

void QMeeGoGraphicsSystemSwitchHandler::addWidget(QWidget *widget)
{
    if (widget != qt_gl_share_widget() && !m_widgets.contains(widget)) {
        widget->installEventFilter(this);
        connect(widget, SIGNAL(destroyed(QObject *)), this, SLOT(removeWidget(QObject *)));
        m_widgets << widget;
    }
}

void QMeeGoGraphicsSystemSwitchHandler::handleMapNotify()
{
    if (m_widgets.isEmpty() && QMeeGoGraphicsSystem::switchPolicy == QMeeGoGraphicsSystem::AutomaticSwitch)
        QTimer::singleShot(0, this, SLOT(switchToMeeGo()));
}

void QMeeGoGraphicsSystemSwitchHandler::removeWidget(QObject *object)
{
    m_widgets.removeOne(static_cast<QWidget *>(object));
    if (m_widgets.isEmpty() && QMeeGoGraphicsSystem::switchPolicy == QMeeGoGraphicsSystem::AutomaticSwitch)
        QTimer::singleShot(0, this, SLOT(switchToRaster()));
}

void QMeeGoGraphicsSystemSwitchHandler::switchToRaster()
{
    QMeeGoGraphicsSystem::switchToRaster();
}

void QMeeGoGraphicsSystemSwitchHandler::switchToMeeGo()
{
    QMeeGoGraphicsSystem::switchToMeeGo();
}

int QMeeGoGraphicsSystemSwitchHandler::visibleWidgets() const
{
    int count = 0;
    for (int i = 0; i < m_widgets.size(); ++i)
        count += m_widgets.at(i)->isVisible() && !(m_widgets.at(i)->windowState() & Qt::WindowMinimized);
    return count;
}

bool QMeeGoGraphicsSystemSwitchHandler::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange
        && QMeeGoGraphicsSystem::switchPolicy == QMeeGoGraphicsSystem::AutomaticSwitch)
    {
        QWindowStateChangeEvent *change = static_cast<QWindowStateChangeEvent *>(event);
        QWidget *widget = static_cast<QWidget *>(object);

        Qt::WindowStates current = widget->windowState();
        Qt::WindowStates old = change->oldState();

        // did minimized flag change?
        if ((current ^ old) & Qt::WindowMinimized) {
            if (current & Qt::WindowMinimized) {
                if (visibleWidgets() == 0)
                    QMeeGoGraphicsSystem::switchToRaster();
            } else {
                if (visibleWidgets() > 0)
                    QMeeGoGraphicsSystem::switchToMeeGo();
            }
        }
    }

    // resume processing of event
    return false;
}

Q_GLOBAL_STATIC(QMeeGoGraphicsSystemSwitchHandler, switch_handler)

bool x11EventFilter(XEvent *event)
{
    if (event->type == MapNotify)
        switch_handler()->handleMapNotify();
    return false;
}

QWindowSurface* QMeeGoGraphicsSystem::createWindowSurface(QWidget *widget) const
{
    QGLWidget *shareWidget = qt_gl_share_widget();

    if (!shareWidget)
        return new QRasterWindowSurface(widget);

    QGLShareContextScope ctx(shareWidget->context());

    if (QApplicationPrivate::instance()->graphics_system_name == QLatin1String("runtime"))
        switch_handler()->addWidget(widget);

    QMeeGoGraphicsSystem::surfaceWasCreated = true;
    QWindowSurface *surface = new QGLWindowSurface(widget);
    return surface;
}

QPixmapData *QMeeGoGraphicsSystem::createPixmapData(QPixmapData::PixelType type) const
{
    return new QRasterPixmapData(type);
}

QPixmapData *QMeeGoGraphicsSystem::createPixmapData(QPixmapData *origin)
{
    // If the pixmap is a raster type...
    // and if the pixmap pointer matches our mapping...
    // create a shared image instead with the given handle.

    if (!origin->isNull() && origin->classId() == QPixmapData::RasterClass) {
        QRasterPixmapData *rasterClass = static_cast <QRasterPixmapData *> (origin);
        void *rawResource = static_cast <void *> (rasterClass->buffer()->data_ptr()->data);

        if (QMeeGoPixmapData::sharedImagesMap.contains(rawResource))
            return new QMeeGoPixmapData();
    }

    return new QRasterPixmapData(origin->pixelType());
}

QPixmapData* QMeeGoGraphicsSystem::wrapPixmapData(QPixmapData *pmd)
{
    QString name = QApplicationPrivate::instance()->graphics_system_name;
    if (name == "runtime") {
        QRuntimeGraphicsSystem *rsystem = (QRuntimeGraphicsSystem *) QApplicationPrivate::instance()->graphics_system;
        QRuntimePixmapData *rt = new QRuntimePixmapData(rsystem, pmd->pixelType());;
        rt->m_data = pmd;
        rt->readBackInfo();
        rsystem->m_pixmapDatas << rt;
        return rt;
    } else
        return pmd;
}

void QMeeGoGraphicsSystem::setSurfaceFixedSize(int /*width*/, int /*height*/)
{
    if (QMeeGoGraphicsSystem::surfaceWasCreated) {
        qWarning("Trying to set surface fixed size but surface already created!");
        return;
    }

#ifdef QT_WAS_PATCHED
    QEglProperties *properties = new QEglProperties();
    properties->setValue(EGL_FIXED_WIDTH_NOK, width);
    properties->setValue(EGL_FIXED_HEIGHT_NOK, height);
    QGLContextPrivate::setExtraWindowSurfaceCreationProps(properties);
#endif
}

void QMeeGoGraphicsSystem::setSurfaceScaling(int x, int y, int width, int height)
{
    QMeeGoExtensions::ensureInitialized();
    QMeeGoExtensions::eglSetSurfaceScalingNOK(QEgl::display(), QEglContext::currentContext(QEgl::OpenGL)->currentSurface, x, y, width, height);
}

void QMeeGoGraphicsSystem::setTranslucent(bool translucent)
{
    if (QMeeGoGraphicsSystem::surfaceWasCreated) {
        qWarning("Trying to set translucency but surface already created!");
        return;
    }

    QGLWindowSurface::surfaceFormat.setSampleBuffers(false);
    QGLWindowSurface::surfaceFormat.setSamples(0);
    QGLWindowSurface::surfaceFormat.setAlpha(translucent);
}

QPixmapData *QMeeGoGraphicsSystem::pixmapDataFromEGLSharedImage(Qt::HANDLE handle, const QImage &softImage)
{
    if (softImage.format() != QImage::Format_ARGB32_Premultiplied &&
        softImage.format() != QImage::Format_RGB32) {
        qFatal("For egl shared images, the soft image has to be ARGB32_Premultiplied or RGB32");
        return NULL;
    }

    if (QMeeGoGraphicsSystem::meeGoRunning()) {
        QMeeGoPixmapData *pmd = new QMeeGoPixmapData;
        pmd->fromEGLSharedImage(handle, softImage);
        return QMeeGoGraphicsSystem::wrapPixmapData(pmd);
    } else {
        QRasterPixmapData *pmd = new QRasterPixmapData(QPixmapData::PixmapType);
        pmd->fromImage(softImage, Qt::NoFormatConversion);

        // Make sure that the image was not converted in any way
        if (pmd->buffer()->data_ptr()->data !=
            const_cast<QImage &>(softImage).data_ptr()->data)
            qFatal("Iternal misalignment of raster data detected. Prolly a QImage copy fail.");

        QMeeGoPixmapData::registerSharedImage(handle, softImage);
        return QMeeGoGraphicsSystem::wrapPixmapData(pmd);
    }
}

void QMeeGoGraphicsSystem::updateEGLSharedImagePixmap(QPixmap *pixmap)
{
    QMeeGoPixmapData *pmd = (QMeeGoPixmapData *) pixmap->pixmapData();

    // Basic sanity check to make sure this is really a QMeeGoPixmapData...
    if (pmd->classId() != QPixmapData::OpenGLClass)
        qFatal("Trying to updated EGLSharedImage pixmap but it's not really a shared image pixmap!");

    pmd->updateFromSoftImage();
}

QPixmapData *QMeeGoGraphicsSystem::pixmapDataWithGLTexture(int w, int h)
{
    QGLPixmapData *pmd = new QGLPixmapData(QPixmapData::PixmapType);
    pmd->resize(w, h);
    return QMeeGoGraphicsSystem::wrapPixmapData(pmd);
}

bool QMeeGoGraphicsSystem::meeGoRunning()
{
    return runningGraphicsSystemName() == "meego";
}

QPixmapData* QMeeGoGraphicsSystem::pixmapDataWithNewLiveTexture(int w, int h, QImage::Format format)
{
    return new QMeeGoLivePixmapData(w, h, format);
}

QPixmapData* QMeeGoGraphicsSystem::pixmapDataFromLiveTextureHandle(Qt::HANDLE handle)
{
    return new QMeeGoLivePixmapData(handle);
}

QImage* QMeeGoGraphicsSystem::lockLiveTexture(QPixmap* pixmap, void* fenceSync)
{
    QMeeGoLivePixmapData *pixmapData = static_cast<QMeeGoLivePixmapData*>(pixmap->data_ptr().data());
    return pixmapData->lock(fenceSync);
}

bool QMeeGoGraphicsSystem::releaseLiveTexture(QPixmap *pixmap, QImage *image)
{
    QMeeGoLivePixmapData *pixmapData = static_cast<QMeeGoLivePixmapData*>(pixmap->data_ptr().data());
    return pixmapData->release(image);
}

Qt::HANDLE QMeeGoGraphicsSystem::getLiveTextureHandle(QPixmap *pixmap)
{
    QMeeGoLivePixmapData *pixmapData = static_cast<QMeeGoLivePixmapData*>(pixmap->data_ptr().data());
    return pixmapData->handle();
}

void* QMeeGoGraphicsSystem::createFenceSync()
{
    QGLShareContextScope ctx(qt_gl_share_widget()->context());
    QMeeGoExtensions::ensureInitialized();
    return QMeeGoExtensions::eglCreateSyncKHR(QEgl::display(), EGL_SYNC_FENCE_KHR, NULL);
}

void QMeeGoGraphicsSystem::destroyFenceSync(void *fenceSync)
{
    QGLShareContextScope ctx(qt_gl_share_widget()->context());
    QMeeGoExtensions::ensureInitialized();
    QMeeGoExtensions::eglDestroySyncKHR(QEgl::display(), fenceSync);
}

QString QMeeGoGraphicsSystem::runningGraphicsSystemName()
{
    if (!QApplicationPrivate::instance()) {
        qWarning("Querying graphics system but application not running yet!");
        return QString();
    }

    QString name = QApplicationPrivate::instance()->graphics_system_name;
    if (name == QLatin1String("runtime")) {
        QRuntimeGraphicsSystem *rsystem = (QRuntimeGraphicsSystem *) QApplicationPrivate::instance()->graphics_system;
        name = rsystem->graphicsSystemName();
    }

    return name;
}

void QMeeGoGraphicsSystem::switchToMeeGo()
{
    if (switchPolicy == NoSwitch || meeGoRunning())
        return;

    if (QApplicationPrivate::instance()->graphics_system_name != QLatin1String("runtime"))
        qWarning("Can't switch to meego - switching only supported with 'runtime' graphics system.");
    else {
        triggerSwitchCallbacks(0, "meego");

        QApplication *app = static_cast<QApplication *>(QCoreApplication::instance());
        app->setGraphicsSystem(QLatin1String("meego"));

        triggerSwitchCallbacks(1, "meego");
    }
}

void QMeeGoGraphicsSystem::switchToRaster()
{
    if (switchPolicy == NoSwitch || runningGraphicsSystemName() == QLatin1String("raster"))
        return;

    if (QApplicationPrivate::instance()->graphics_system_name != QLatin1String("runtime"))
        qWarning("Can't switch to raster - switching only supported with 'runtime' graphics system.");
    else {
        triggerSwitchCallbacks(0, "raster");

        QApplication *app = static_cast<QApplication *>(QCoreApplication::instance());
        app->setGraphicsSystem(QLatin1String("raster"));

        QMeeGoLivePixmapData::invalidateSurfaces();

        triggerSwitchCallbacks(1, "raster");
    }
}

void QMeeGoGraphicsSystem::registerSwitchCallback(QMeeGoSwitchCallback callback)
{
    switchCallbacks << callback;
}

void QMeeGoGraphicsSystem::triggerSwitchCallbacks(int type, const char *name)
{
    for (int i = 0; i < switchCallbacks.size(); ++i)
        switchCallbacks.at(i)(type, name);
}

/* C API */

int qt_meego_image_to_egl_shared_image(const QImage &image)
{
    return QMeeGoPixmapData::imageToEGLSharedImage(image);
}

QPixmapData* qt_meego_pixmapdata_from_egl_shared_image(Qt::HANDLE handle, const QImage &softImage)
{
    return QMeeGoGraphicsSystem::pixmapDataFromEGLSharedImage(handle, softImage);
}

QPixmapData* qt_meego_pixmapdata_with_gl_texture(int w, int h)
{
    return QMeeGoGraphicsSystem::pixmapDataWithGLTexture(w, h);
}

bool qt_meego_destroy_egl_shared_image(Qt::HANDLE handle)
{
    return QMeeGoPixmapData::destroyEGLSharedImage(handle);
}

void qt_meego_set_surface_fixed_size(int width, int height)
{
    QMeeGoGraphicsSystem::setSurfaceFixedSize(width, height);
}

void qt_meego_set_surface_scaling(int x, int y, int width, int height)
{
    QMeeGoGraphicsSystem::setSurfaceScaling(x, y, width, height);
}

void qt_meego_set_translucent(bool translucent)
{
    QMeeGoGraphicsSystem::setTranslucent(translucent);
}

void qt_meego_update_egl_shared_image_pixmap(QPixmap *pixmap)
{
    QMeeGoGraphicsSystem::updateEGLSharedImagePixmap(pixmap);
}

QPixmapData* qt_meego_pixmapdata_with_new_live_texture(int w, int h, QImage::Format format)
{
    return QMeeGoGraphicsSystem::pixmapDataWithNewLiveTexture(w, h, format);
}

QPixmapData* qt_meego_pixmapdata_from_live_texture_handle(Qt::HANDLE handle)
{
    return QMeeGoGraphicsSystem::pixmapDataFromLiveTextureHandle(handle);
}

QImage* qt_meego_live_texture_lock(QPixmap *pixmap, void *fenceSync)
{
    return QMeeGoGraphicsSystem::lockLiveTexture(pixmap, fenceSync);
}

bool qt_meego_live_texture_release(QPixmap *pixmap, QImage *image)
{
    return QMeeGoGraphicsSystem::releaseLiveTexture(pixmap, image);
}

Qt::HANDLE qt_meego_live_texture_get_handle(QPixmap *pixmap)
{
    return QMeeGoGraphicsSystem::getLiveTextureHandle(pixmap);
}

void* qt_meego_create_fence_sync(void)
{
    return QMeeGoGraphicsSystem::createFenceSync();
}

void qt_meego_destroy_fence_sync(void* fs)
{
    return QMeeGoGraphicsSystem::destroyFenceSync(fs);
}

void qt_meego_invalidate_live_surfaces(void)
{
    return QMeeGoLivePixmapData::invalidateSurfaces();
}

void qt_meego_switch_to_raster(void)
{
    QMeeGoGraphicsSystem::switchToRaster();
}

void qt_meego_switch_to_meego(void)
{
    QMeeGoGraphicsSystem::switchToMeeGo();
}

void qt_meego_register_switch_callback(QMeeGoSwitchCallback callback)
{
    QMeeGoGraphicsSystem::registerSwitchCallback(callback);
}

void qt_meego_set_switch_policy(int policy)
{
    QMeeGoGraphicsSystem::switchPolicy = QMeeGoGraphicsSystem::SwitchPolicy(policy);
}

#include "qmeegographicssystem.moc"
