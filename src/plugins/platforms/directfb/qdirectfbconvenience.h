// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDIRECTFBCONVENIENCE_H
#define QDIRECTFBCONVENIENCE_H

#include <QtGui/qimage.h>
#include <QtCore/QHash>
#include <QtCore/QEvent>
#include <QtGui/QPixmap>

#include <directfb.h>

QT_BEGIN_NAMESPACE

class QDirectFbScreen;
class QPlatformScreen;

class QDirectFbKeyMap: public QHash<DFBInputDeviceKeySymbol, Qt::Key>
{
public:
    QDirectFbKeyMap();
};


class QDirectFbConvenience
{
public:
    static QImage::Format imageFormatFromSurfaceFormat(const DFBSurfacePixelFormat format, const DFBSurfaceCapabilities caps);
    static bool pixelFomatHasAlpha(const DFBSurfacePixelFormat format) { return (1 << 16) & format; }
    static int colorDepthForSurface(const DFBSurfacePixelFormat format);

    //This is set by the graphicssystem constructor
    static IDirectFB *dfbInterface();
    static IDirectFBDisplayLayer *dfbDisplayLayer(int display = DLID_PRIMARY);

    static IDirectFBSurface *dfbSurfaceForPlatformPixmap(QPlatformPixmap *);

    static Qt::MouseButton mouseButton(DFBInputDeviceButtonIdentifier identifier);
    static Qt::MouseButtons mouseButtons(DFBInputDeviceButtonMask mask);
    static Qt::KeyboardModifiers keyboardModifiers(DFBInputDeviceModifierMask mask);
    static QEvent::Type eventType(DFBWindowEventType type);

    static QDirectFbKeyMap *keyMap();

private:
    static QDirectFbKeyMap *dfbKeymap;
    friend class QDirectFbIntegration;
};

template <typename T> struct QDirectFBInterfaceCleanupHandler
{
    static void cleanup(T *t)
    {
        if (!t)
            return;
        t->Release(t);
    }
};

template <typename T>
class QDirectFBPointer : public QScopedPointer<T, QDirectFBInterfaceCleanupHandler<T> >
{
public:
    Q_NODISCARD_CTOR QDirectFBPointer(T *t = nullptr)
        : QScopedPointer<T, QDirectFBInterfaceCleanupHandler<T> >(t)
    {}

    T** outPtr()
    {
        this->reset(nullptr);
        return &this->d;
    }
};

// Helper conversions from internal to DFB types
QDirectFbScreen *toDfbScreen(QWindow *window);
IDirectFBDisplayLayer *toDfbLayer(QPlatformScreen *screen);

#define QDFB_STRINGIFY(x) #x
#define QDFB_TOSTRING(x) QDFB_STRINGIFY(x)
#define QDFB_PRETTY \
    (__FILE__ ":" QDFB_TOSTRING(__LINE__))

QT_END_NAMESPACE


#endif // QDIRECTFBCONVENIENCE_H
