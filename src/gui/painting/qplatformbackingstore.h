/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPLATFORMBACKINGSTORE_H
#define QPLATFORMBACKINGSTORE_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qrect.h>
#include <QtCore/qobject.h>

#include <QtGui/qwindow.h>
#include <QtGui/qregion.h>

QT_BEGIN_NAMESPACE

Q_GUI_EXPORT Q_DECLARE_LOGGING_CATEGORY(lcQpaBackingStore)

class QRegion;
class QRect;
class QPoint;
class QImage;
class QPlatformBackingStorePrivate;
class QPlatformTextureList;
class QPlatformTextureListPrivate;
class QPlatformGraphicsBuffer;
class QRhi;
class QRhiTexture;
class QRhiResourceUpdateBatch;
class QRhiSwapChain;

struct Q_GUI_EXPORT QPlatformBackingStoreRhiConfig
{
    enum Api {
        OpenGL,
        Metal,
        Vulkan,
        D3D11,
        Null
    };

    QPlatformBackingStoreRhiConfig()
        : m_enable(false)
    { }

    QPlatformBackingStoreRhiConfig(Api api)
        : m_enable(true),
          m_api(api)
    { }

    bool isEnabled() const { return m_enable; }
    void setEnabled(bool enable) { m_enable = enable; }

    Api api() const { return m_api; }
    void setApi(Api api) { m_api = api; }

    bool isDebugLayerEnabled() const { return m_debugLayer; }
    void setDebugLayer(bool enable) { m_debugLayer = enable; }

    QByteArrayList instanceExtensions() const { return m_instanceExtensions; }
    void setInstanceExtensions(const QByteArrayList &e) { m_instanceExtensions = e; }

    QByteArrayList instanceLayers() const { return m_instanceLayers; }
    void setInstanceLayers(const QByteArrayList &e) { m_instanceLayers = e; }

    QByteArrayList deviceExtensions() const { return m_deviceExtensions; }
    void setDeviceExtensions(const QByteArrayList &e) { m_deviceExtensions = e; }

private:
    bool m_enable;
    Api m_api = Null;
    bool m_debugLayer = false;
    QByteArrayList m_instanceExtensions;
    QByteArrayList m_instanceLayers;
    QByteArrayList m_deviceExtensions;
    friend bool operator==(const QPlatformBackingStoreRhiConfig &a, const QPlatformBackingStoreRhiConfig &b);
};

inline bool operator==(const QPlatformBackingStoreRhiConfig &a, const QPlatformBackingStoreRhiConfig &b)
{
    return a.m_enable == b.m_enable
            && a.m_api == b.m_api
            && a.m_debugLayer == b.m_debugLayer
            && a.m_instanceExtensions == b.m_instanceExtensions
            && a.m_instanceLayers == b.m_instanceLayers
            && a.m_deviceExtensions == b.m_deviceExtensions;
}

inline bool operator!=(const QPlatformBackingStoreRhiConfig &a, const QPlatformBackingStoreRhiConfig &b)
{
    return !(a == b);
}

class Q_GUI_EXPORT QPlatformTextureList : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPlatformTextureList)
public:
    enum Flag {
        StacksOnTop = 0x01,
        TextureIsSrgb = 0x02,
        NeedsPremultipliedAlphaBlending = 0x04
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    explicit QPlatformTextureList(QObject *parent = nullptr);
    ~QPlatformTextureList();

    int count() const;
    bool isEmpty() const { return count() == 0; }
    QRhiTexture *texture(int index) const;
    QRect geometry(int index) const;
    QRect clipRect(int index) const;
    void *source(int index);
    Flags flags(int index) const;
    void lock(bool on);
    bool isLocked() const;

    void appendTexture(void *source, QRhiTexture *texture, const QRect &geometry,
                       const QRect &clipRect = QRect(), Flags flags = { });
    void clear();

 Q_SIGNALS:
    void locked(bool);
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QPlatformTextureList::Flags)

class Q_GUI_EXPORT QPlatformBackingStore
{
public:
    enum FlushResult {
        FlushSuccess,
        FlushFailed,
        FlushFailedDueToLostDevice
    };

    explicit QPlatformBackingStore(QWindow *window);
    virtual ~QPlatformBackingStore();

    QWindow *window() const;
    QBackingStore *backingStore() const;

    virtual QPaintDevice *paintDevice() = 0;

    virtual void flush(QWindow *window, const QRegion &region, const QPoint &offset);

    virtual FlushResult rhiFlush(QWindow *window,
                                 const QRegion &region,
                                 const QPoint &offset,
                                 QPlatformTextureList *textures,
                                 bool translucentBackground);

    virtual QImage toImage() const;

    enum TextureFlag {
        TextureSwizzle = 0x01,
        TextureFlip = 0x02,
        TexturePremultiplied = 0x04
    };
    Q_DECLARE_FLAGS(TextureFlags, TextureFlag)
    virtual QRhiTexture *toTexture(QRhiResourceUpdateBatch *resourceUpdates,
                                   const QRegion &dirtyRegion,
                                   TextureFlags *flags) const;

    virtual QPlatformGraphicsBuffer *graphicsBuffer() const;

    virtual void resize(const QSize &size, const QRegion &staticContents) = 0;

    virtual bool scroll(const QRegion &area, int dx, int dy);

    virtual void beginPaint(const QRegion &);
    virtual void endPaint();

    void setRhiConfig(const QPlatformBackingStoreRhiConfig &config);
    QRhi *rhi() const;
    QRhiSwapChain *rhiSwapChain() const;
    void surfaceAboutToBeDestroyed();
    void graphicsDeviceReportedLost();

private:
    QPlatformBackingStorePrivate *d_ptr;

    void setBackingStore(QBackingStore *);
    friend class QBackingStore;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QPlatformBackingStore::TextureFlags)

QT_END_NAMESPACE

#endif // QPLATFORMBACKINGSTORE_H
