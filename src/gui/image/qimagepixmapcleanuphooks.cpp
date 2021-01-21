/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qimagepixmapcleanuphooks_p.h"
#include <qpa/qplatformpixmap.h>
#include "private/qimage_p.h"


QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QImagePixmapCleanupHooks, qt_image_and_pixmap_cleanup_hooks)

QImagePixmapCleanupHooks *QImagePixmapCleanupHooks::instance()
{
    return qt_image_and_pixmap_cleanup_hooks();
}

void QImagePixmapCleanupHooks::addPlatformPixmapModificationHook(_qt_pixmap_cleanup_hook_pmd hook)
{
    pixmapModificationHooks.append(hook);
}

void QImagePixmapCleanupHooks::addPlatformPixmapDestructionHook(_qt_pixmap_cleanup_hook_pmd hook)
{
    pixmapDestructionHooks.append(hook);
}


void QImagePixmapCleanupHooks::addImageHook(_qt_image_cleanup_hook_64 hook)
{
    imageHooks.append(hook);
}

void QImagePixmapCleanupHooks::removePlatformPixmapModificationHook(_qt_pixmap_cleanup_hook_pmd hook)
{
    pixmapModificationHooks.removeAll(hook);
}

void QImagePixmapCleanupHooks::removePlatformPixmapDestructionHook(_qt_pixmap_cleanup_hook_pmd hook)
{
    pixmapDestructionHooks.removeAll(hook);
}

void QImagePixmapCleanupHooks::removeImageHook(_qt_image_cleanup_hook_64 hook)
{
    imageHooks.removeAll(hook);
}

void QImagePixmapCleanupHooks::executePlatformPixmapModificationHooks(QPlatformPixmap* pmd)
{
    const QImagePixmapCleanupHooks *h = qt_image_and_pixmap_cleanup_hooks();
    // the global destructor for the pixmap and image hooks might have
    // been called already if the app is "leaking" global
    // pixmaps/images
    if (!h)
        return;
    for (auto hook : h->pixmapModificationHooks)
        hook(pmd);
}

void QImagePixmapCleanupHooks::executePlatformPixmapDestructionHooks(QPlatformPixmap* pmd)
{
    const QImagePixmapCleanupHooks *h = qt_image_and_pixmap_cleanup_hooks();
    // the global destructor for the pixmap and image hooks might have
    // been called already if the app is "leaking" global
    // pixmaps/images
    if (!h)
        return;
    for (auto hook : h->pixmapDestructionHooks)
        hook(pmd);
}

void QImagePixmapCleanupHooks::executeImageHooks(qint64 key)
{
    const QImagePixmapCleanupHooks *h = qt_image_and_pixmap_cleanup_hooks();
    // the global destructor for the pixmap and image hooks might have
    // been called already if the app is "leaking" global
    // pixmaps/images
    if (!h)
        return;
    for (auto hook : h->imageHooks)
        hook(key);
}


void QImagePixmapCleanupHooks::enableCleanupHooks(QPlatformPixmap *handle)
{
    handle->is_cached = true;
}

void QImagePixmapCleanupHooks::enableCleanupHooks(const QPixmap &pixmap)
{
    enableCleanupHooks(const_cast<QPixmap &>(pixmap).data_ptr().data());
}

void QImagePixmapCleanupHooks::enableCleanupHooks(const QImage &image)
{
    const_cast<QImage &>(image).data_ptr()->is_cached = true;
}

bool QImagePixmapCleanupHooks::isImageCached(const QImage &image)
{
    return const_cast<QImage &>(image).data_ptr()->is_cached;
}

bool QImagePixmapCleanupHooks::isPixmapCached(const QPixmap &pixmap)
{
    return const_cast<QPixmap&>(pixmap).data_ptr().data()->is_cached;
}



QT_END_NAMESPACE
