// Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qeglfskmsgbmdevice_p.h"
#include "qeglfskmsgbmscreen_p.h"

#include <private/qeglfsintegration_p.h>
#include <private/qeglfskmsintegration_p.h>

#include <QtCore/QLoggingCategory>
#include <QtCore/qtimer.h>
#include <QtCore/private/qcore_unix_p.h>

#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])

QT_BEGIN_NAMESPACE

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


    // On some platforms (e.g. rpi4), you'll get a kernel warning/error
    // if the cursor is created 'at the same time' as the screen is created.
    // (drmModeMoveCursor is the specific call that causes the issue)
    // When this issue is triggered, the screen's connector is unusable until reboot
    //
    // Below is a work-around (without negative implications for other platforms).
    //
    // interval of 0 and QMetaObject::invokeMethod (w/o Qt::QueuedConnection)
    // do no help / will still trigger issue
    QTimer::singleShot(1, [screen, this](){
        createGlobalCursor(screen);
    });

    return screen;
}

QPlatformScreen *QEglFSKmsGbmDevice::createHeadlessScreen()
{
    destroyGlobalCursor();

    return new QEglFSKmsGbmScreen(this, QKmsOutput(), true);
}

void QEglFSKmsGbmDevice::registerScreenCloning(QPlatformScreen *screen,
                                               QPlatformScreen *screenThisScreenClones,
                                               const QList<QPlatformScreen *> &screensCloningThisScreen)
{
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

void QEglFSKmsGbmDevice::unregisterScreen(QPlatformScreen *screen)
{
    // The global cursor holds a pointer to a QEglFSKmsGbmScreen.
    // If that screen is being unregistered,
    // this will recreate the global cursor with the first sibling screen.
    if (m_globalCursor && screen == m_globalCursor->screen()) {
        qCDebug(qLcEglfsKmsDebug) << "Destroying global GBM mouse cursor due to unregistering"
                                  << "it's screen - will probably be recreated right away";
        delete m_globalCursor;
        m_globalCursor = nullptr;

        QList<QPlatformScreen *> siblings = screen->virtualSiblings();
        siblings.removeOne(screen);
        if (siblings.count() > 0) {
            QEglFSKmsGbmScreen *kmsScreen = static_cast<QEglFSKmsGbmScreen *>(siblings.first());
            m_globalCursor = new QEglFSKmsGbmCursor(kmsScreen);
            qCDebug(qLcEglfsKmsDebug) << "Creating new global GBM mouse cursor on sibling screen";
        } else {
            qCWarning(qLcEglfsKmsDebug) << "Couldn't find a sibling to recreate"
                                        << "the GBM mouse cursor - it might vanish";
        }
    }

    QEglFSKmsDevice::unregisterScreen(screen);
}

bool QEglFSKmsGbmDevice::usesEventReader() const
{
    static const bool eventReaderThreadDisabled = qEnvironmentVariableIntValue("QT_QPA_EGLFS_KMS_NO_EVENT_READER_THREAD");
    return !eventReaderThreadDisabled;
}

QT_END_NAMESPACE
