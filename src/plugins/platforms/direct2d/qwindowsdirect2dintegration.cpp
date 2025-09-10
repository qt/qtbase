// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qt_windows.h>
#include "qwindowsdirect2dcontext.h"
#include "qwindowsdirect2dintegration.h"
#include "qwindowsdirect2dbackingstore.h"
#include "qwindowsdirect2dplatformpixmap.h"
#include "qwindowsdirect2dnativeinterface.h"
#include "qwindowsdirect2dwindow.h"

#include "qwindowscontext.h"

#include <qplatformdefs.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qversionnumber.h>
#include <QtGui/private/qpixmap_raster_p.h>
#include <QtGui/qpa/qwindowsysteminterface.h>

#include <QVarLengthArray>

QT_BEGIN_NAMESPACE

class QWindowsDirect2DIntegrationPrivate
{
public:
    QWindowsDirect2DNativeInterface m_nativeInterface;
    QWindowsDirect2DContext m_d2dContext;
};

static QVersionNumber systemD2DVersion()
{
    static const int bufSize = 512;
    TCHAR filename[bufSize];

    UINT i = GetSystemDirectory(filename, bufSize);
    if (i > 0 && i < bufSize) {
        if (_tcscat_s(filename, bufSize, __TEXT("\\d2d1.dll")) == 0) {
            DWORD versionInfoSize = GetFileVersionInfoSize(filename, nullptr);
            if (versionInfoSize) {
                QVarLengthArray<BYTE> info(static_cast<int>(versionInfoSize));
                if (GetFileVersionInfo(filename, 0, versionInfoSize, info.data())) {
                    UINT size;
                    DWORD *fi;

                    if (VerQueryValue(info.constData(), __TEXT("\\"),
                                      reinterpret_cast<void **>(&fi), &size) && size) {
                        const auto *verInfo = reinterpret_cast<const VS_FIXEDFILEINFO *>(fi);
                        return QVersionNumber{HIWORD(verInfo->dwFileVersionMS), LOWORD(verInfo->dwFileVersionMS),
                                              HIWORD(verInfo->dwFileVersionLS), LOWORD(verInfo->dwFileVersionLS)};
                    }
                }
            }
        }
    }
    return QVersionNumber();
}

static QVersionNumber minimumD2DVersion()
{
    // 6.2.9200.16492 corresponds to Direct2D 1.1 on Windows 7 SP1 with Platform Update
    enum : int {
        D2DMinVersionPart1 = 6,
        D2DMinVersionPart2 = 2,
        D2DMinVersionPart3 = 9200,
        D2DMinVersionPart4 = 16492
    };

    return QVersionNumber{D2DMinVersionPart1, D2DMinVersionPart2, D2DMinVersionPart3, D2DMinVersionPart4};
}

QWindowsDirect2DIntegration *QWindowsDirect2DIntegration::create(const QStringList &paramList)
{
    const QVersionNumber systemVersion = systemD2DVersion();
    const QVersionNumber minimumVersion = minimumD2DVersion();
    if (!systemVersion.isNull() && systemVersion < minimumVersion) {
        QString msg = QCoreApplication::translate("QWindowsDirect2DIntegration",
            "Qt cannot load the direct2d platform plugin because " \
            "the Direct2D version on this system is too old. The " \
            "minimum system requirement for this platform plugin " \
            "is Windows 7 SP1 with Platform Update.\n\n" \
            "The minimum Direct2D version required is %1. " \
            "The Direct2D version on this system is %2.")
            .arg(minimumVersion.toString(), systemVersion.toString());

        QString caption = QCoreApplication::translate("QWindowsDirect2DIntegration",
            "Cannot load direct2d platform plugin");

        MessageBoxW(nullptr,
                    msg.toStdWString().c_str(),
                    caption.toStdWString().c_str(),
                    MB_OK | MB_ICONERROR);

        return nullptr;
    }

    auto *integration = new QWindowsDirect2DIntegration(paramList);

    if (!integration->init()) {
        delete integration;
        integration = nullptr;
    }

    return integration;
}

QWindowsDirect2DIntegration::~QWindowsDirect2DIntegration()
{

}

 QWindowsDirect2DIntegration *QWindowsDirect2DIntegration::instance()
 {
     return static_cast<QWindowsDirect2DIntegration *>(QWindowsIntegration::instance());
 }


QWindowsWindow *QWindowsDirect2DIntegration::createPlatformWindowHelper(QWindow *window, const QWindowsWindowData &data) const
{
    return new QWindowsDirect2DWindow(window, data);
}

 QPlatformNativeInterface *QWindowsDirect2DIntegration::nativeInterface() const
 {
     return &d->m_nativeInterface;
 }

QPlatformPixmap *QWindowsDirect2DIntegration::createPlatformPixmap(QPlatformPixmap::PixelType type) const
{
    switch (type) {
    case QPlatformPixmap::BitmapType:
        return new QRasterPlatformPixmap(type);
        break;
    default:
        return new QWindowsDirect2DPlatformPixmap(type);
        break;
    }
}

QPlatformBackingStore *QWindowsDirect2DIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QWindowsDirect2DBackingStore(window);
}

QWindowsDirect2DContext *QWindowsDirect2DIntegration::direct2DContext() const
{
    return &d->m_d2dContext;
}

QWindowsDirect2DIntegration::QWindowsDirect2DIntegration(const QStringList &paramList)
    : QWindowsIntegration(paramList)
    , d(new QWindowsDirect2DIntegrationPrivate)
{
}

bool QWindowsDirect2DIntegration::init()
{
    return d->m_d2dContext.init();
}

QT_END_NAMESPACE
