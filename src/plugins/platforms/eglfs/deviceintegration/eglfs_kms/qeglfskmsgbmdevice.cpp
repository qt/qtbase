// Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeglfskmsgbmdevice_p.h"
#include "qeglfskmsgbmscreen_p.h"

#include <private/qeglfsintegration_p.h>

#include <QtCore/QLoggingCategory>
#include <QtCore/private/qcore_unix_p.h>

#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcEglfsKmsDebug)

QEglFSKmsGbmDevice::QEglFSKmsGbmDevice(QKmsScreenConfig *screenConfig, const QString &path)
    : QEglFSKmsDevice(screenConfig, path)
    , m_gbm_device(nullptr)
    , m_globalCursor(nullptr)
{
}

bool QEglFSKmsGbmDevice::open()
{
    Q_ASSERT(fd() == -1);
    Q_ASSERT(m_gbm_device == nullptr);

    int fd = qt_safe_open(devicePath().toLocal8Bit().constData(), O_RDWR | O_CLOEXEC);
    if (fd == -1) {
        qErrnoWarning("Could not open DRM device %s", qPrintable(devicePath()));
        return false;
    }

    qCDebug(qLcEglfsKmsDebug) << "Creating GBM device for file descriptor" << fd
                              << "obtained from" << devicePath();
    m_gbm_device = gbm_create_device(fd);
    if (!m_gbm_device) {
        qErrnoWarning("Could not create GBM device");
        qt_safe_close(fd);
        fd = -1;
        return false;
    }

    setFd(fd);

    if (usesEventReader()) {
        qCDebug(qLcEglfsKmsDebug, "Using dedicated drm event reading thread");
        m_eventReader.create(this);
    } else {
        qCDebug(qLcEglfsKmsDebug, "Not using dedicated drm event reading thread; "
                "threaded multi-screen setups may experience problems");
    }

    return true;
}

void QEglFSKmsGbmDevice::close()
{
    // Note: screens are gone at this stage.

    if (usesEventReader())
        m_eventReader.destroy();

    if (m_gbm_device) {
        gbm_device_destroy(m_gbm_device);
        m_gbm_device = nullptr;
    }

    if (fd() != -1) {
        qt_safe_close(fd());
        setFd(-1);
    }
}

void *QEglFSKmsGbmDevice::nativeDisplay() const
{
    return m_gbm_device;
}

gbm_device * QEglFSKmsGbmDevice::gbmDevice() const
{
    return m_gbm_device;
}

QPlatformCursor *QEglFSKmsGbmDevice::globalCursor() const
{
    return m_globalCursor;
}

// Cannot do this from close(), it may be too late.
// Call this from the last screen dtor instead.
void QEglFSKmsGbmDevice::destroyGlobalCursor()
{
    if (m_globalCursor) {
        qCDebug(qLcEglfsKmsDebug, "Destroying global GBM mouse cursor");
        delete m_globalCursor;
        m_globalCursor = nullptr;
    }
}

void QEglFSKmsGbmDevice::createGlobalCursor(QEglFSKmsGbmScreen *screen)
{
    if (!m_globalCursor && screenConfig()->hwCursor()) {
        qCDebug(qLcEglfsKmsDebug, "Creating new global GBM mouse cursor");
        m_globalCursor = new QEglFSKmsGbmCursor(screen);
    }
}

QPlatformScreen *QEglFSKmsGbmDevice::createScreen(const QKmsOutput &output)
{
    QEglFSKmsGbmScreen *screen = new QEglFSKmsGbmScreen(this, output, false);

    createGlobalCursor(screen);

    return screen;
}

QPlatformScreen *QEglFSKmsGbmDevice::createHeadlessScreen()
{
    return new QEglFSKmsGbmScreen(this, QKmsOutput(), true);
}

void QEglFSKmsGbmDevice::registerScreenCloning(QPlatformScreen *screen,
                                               QPlatformScreen *screenThisScreenClones,
                                               const QList<QPlatformScreen *> &screensCloningThisScreen)
{
    if (!screenThisScreenClones && screensCloningThisScreen.isEmpty())
        return;

    QEglFSKmsGbmScreen *gbmScreen = static_cast<QEglFSKmsGbmScreen *>(screen);
    gbmScreen->initCloning(screenThisScreenClones, screensCloningThisScreen);
}

void QEglFSKmsGbmDevice::registerScreen(QPlatformScreen *screen,
                                        bool isPrimary,
                                        const QPoint &virtualPos,
                                        const QList<QPlatformScreen *> &virtualSiblings)
{
    QEglFSKmsDevice::registerScreen(screen, isPrimary, virtualPos, virtualSiblings);
    if (screenConfig()->hwCursor() && m_globalCursor)
        m_globalCursor->reevaluateVisibilityForScreens();
}

bool QEglFSKmsGbmDevice::usesEventReader() const
{
    static const bool eventReaderThreadDisabled = qEnvironmentVariableIntValue("QT_QPA_EGLFS_KMS_NO_EVENT_READER_THREAD");
    return !eventReaderThreadDisabled;
}

QT_END_NAMESPACE
