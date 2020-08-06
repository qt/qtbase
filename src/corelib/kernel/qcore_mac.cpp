/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include <private/qcore_mac_p.h>
#include <new>

#include "qhash.h"
#include "qpair.h"
#include "qmutex.h"
#include "qvarlengtharray.h"

#include <dlfcn.h>
#include <mach-o/dyld.h>

QT_BEGIN_NAMESPACE

QCFString::operator QString() const
{
    if (string.isEmpty() && value)
        const_cast<QCFString*>(this)->string = QString::fromCFString(value);
    return string;
}

QCFString::operator CFStringRef() const
{
    if (!value)
        const_cast<QCFString*>(this)->value = string.toCFString();
    return value;
}

// --------------------------------------------------------------------------

#if defined(QT_USE_APPLE_UNIFIED_LOGGING)

bool AppleUnifiedLogger::willMirrorToStderr()
{
    // When running under Xcode or LLDB, one or more of these variables will
    // be set, which triggers libsystem_trace.dyld to log messages to stderr
    // as well, via_os_log_impl_mirror_to_stderr. Un-setting these variables
    // is not an option, as that would silence normal NSLog or os_log calls,
    // so instead we skip our own stderr output. See rdar://36919139.
    static bool willMirror = qEnvironmentVariableIsSet("OS_ACTIVITY_DT_MODE")
                                 || qEnvironmentVariableIsSet("ACTIVITY_LOG_STDERR")
                                 || qEnvironmentVariableIsSet("CFLOG_FORCE_STDERR");
    return willMirror;
}

QT_MAC_WEAK_IMPORT(_os_log_default);
bool AppleUnifiedLogger::messageHandler(QtMsgType msgType, const QMessageLogContext &context,
                                        const QString &message, const QString &optionalSubsystem)
{
    QString subsystem = optionalSubsystem;
    if (subsystem.isNull()) {
        static QString bundleIdentifier = []() {
            if (CFBundleRef bundle = CFBundleGetMainBundle()) {
                if (CFStringRef identifier = CFBundleGetIdentifier(bundle))
                    return QString::fromCFString(identifier);
            }
            return QString();
        }();
        subsystem = bundleIdentifier;
    }

    const bool isDefault = !context.category || !strcmp(context.category, "default");
    os_log_t log = isDefault ? OS_LOG_DEFAULT :
        cachedLog(subsystem, QString::fromLatin1(context.category));
    os_log_type_t logType = logTypeForMessageType(msgType);

    if (!os_log_type_enabled(log, logType))
        return false;

    // Logging best practices says we should not include symbolication
    // information or source file line numbers in messages, as the system
    // will automatically captures this information. In our case, what
    // the system captures is the call to os_log_with_type below, which
    // isn't really useful, but we still don't want to include the context's
    // info, as that would clutter the logging output. See rdar://35958308.

    // The format must be a string constant, so we can't pass on the
    // message. This means we won't be able to take advantage of the
    // unified logging's custom format specifiers such as %{BOOL}d.
    // We use the 'public' format specifier to prevent the logging
    // system from redacting our log message.
    os_log_with_type(log, logType, "%{public}s", qPrintable(message));

    return willMirrorToStderr();
}

os_log_type_t AppleUnifiedLogger::logTypeForMessageType(QtMsgType msgType)
{
    switch (msgType) {
    case QtDebugMsg: return OS_LOG_TYPE_DEBUG;
    case QtInfoMsg: return OS_LOG_TYPE_INFO;
    case QtWarningMsg: return OS_LOG_TYPE_DEFAULT;
    case QtCriticalMsg: return OS_LOG_TYPE_ERROR;
    case QtFatalMsg: return OS_LOG_TYPE_FAULT;
    }

    return OS_LOG_TYPE_DEFAULT;
}

os_log_t AppleUnifiedLogger::cachedLog(const QString &subsystem, const QString &category)
{
    static QBasicMutex mutex;
    QMutexLocker locker(&mutex);

    static QHash<QPair<QString, QString>, os_log_t> logs;
    const auto cacheKey = qMakePair(subsystem, category);
    os_log_t log = logs.value(cacheKey);

    if (!log) {
        log = os_log_create(subsystem.toLatin1().constData(),
            category.toLatin1().constData());
        logs.insert(cacheKey, log);

        // Technically we should release the os_log_t resource when done
        // with it, but since we don't know when a category is disabled
        // we keep all cached os_log_t instances until shutdown, where
        // the OS will clean them up for us.
    }

    return log;
}

#endif // QT_USE_APPLE_UNIFIED_LOGGING

// --------------------------------------------------------------------------

QOperatingSystemVersion QMacVersion::buildSDK(VersionTarget target)
{
    switch (target) {
    case ApplicationBinary: return applicationVersion().second;
    case QtLibraries: return libraryVersion().second;
    }
    Q_UNREACHABLE();
}

QOperatingSystemVersion QMacVersion::deploymentTarget(VersionTarget target)
{
    switch (target) {
    case ApplicationBinary: return applicationVersion().first;
    case QtLibraries: return libraryVersion().first;
    }
    Q_UNREACHABLE();
}

QOperatingSystemVersion QMacVersion::currentRuntime()
{
    return QOperatingSystemVersion::current();
}

// Mach-O platforms
enum Platform {
   macOS = 1,
   iOS = 2,
   tvOS = 3,
   watchOS = 4,
   bridgeOS = 5,
   macCatalyst = 6,
   iOSSimulator = 7,
   tvOSSimulator = 8,
   watchOSSimulator = 9
};

QMacVersion::VersionTuple QMacVersion::versionsForImage(const mach_header *machHeader)
{
    static auto osForLoadCommand = [](uint32_t cmd) {
        switch (cmd) {
        case LC_VERSION_MIN_MACOSX: return QOperatingSystemVersion::MacOS;
        case LC_VERSION_MIN_IPHONEOS: return QOperatingSystemVersion::IOS;
        case LC_VERSION_MIN_TVOS: return QOperatingSystemVersion::TvOS;
        case LC_VERSION_MIN_WATCHOS: return QOperatingSystemVersion::WatchOS;
        default: return QOperatingSystemVersion::Unknown;
        }
    };

    static auto osForPlatform = [](uint32_t platform) {
        switch (platform) {
        case Platform::macOS:
            return QOperatingSystemVersion::MacOS;
        case Platform::iOS:
        case Platform::iOSSimulator:
            return QOperatingSystemVersion::IOS;
        case Platform::tvOS:
        case Platform::tvOSSimulator:
            return QOperatingSystemVersion::TvOS;
        case Platform::watchOS:
        case Platform::watchOSSimulator:
            return QOperatingSystemVersion::WatchOS;
        default:
            return QOperatingSystemVersion::Unknown;
        }
    };

    static auto makeVersionTuple = [](uint32_t dt, uint32_t sdk, QOperatingSystemVersion::OSType osType) {
        return qMakePair(
            QOperatingSystemVersion(osType, dt >> 16 & 0xffff, dt >> 8 & 0xff, dt & 0xff),
            QOperatingSystemVersion(osType, sdk >> 16 & 0xffff, sdk >> 8 & 0xff, sdk & 0xff)
        );
    };

    const bool is64Bit = machHeader->magic == MH_MAGIC_64 || machHeader->magic == MH_CIGAM_64;
    auto commandCursor = uintptr_t(machHeader) + (is64Bit ? sizeof(mach_header_64) : sizeof(mach_header));

    for (uint32_t i = 0; i < machHeader->ncmds; ++i) {
        load_command *loadCommand = reinterpret_cast<load_command *>(commandCursor);
        if (loadCommand->cmd == LC_VERSION_MIN_MACOSX || loadCommand->cmd == LC_VERSION_MIN_IPHONEOS
            || loadCommand->cmd == LC_VERSION_MIN_TVOS || loadCommand->cmd == LC_VERSION_MIN_WATCHOS) {
            auto versionCommand = reinterpret_cast<version_min_command *>(loadCommand);
            return makeVersionTuple(versionCommand->version, versionCommand->sdk, osForLoadCommand(loadCommand->cmd));
#if QT_DARWIN_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_13, __IPHONE_11_0, __TVOS_11_0, __WATCHOS_4_0)
        } else if (loadCommand->cmd == LC_BUILD_VERSION) {
            auto versionCommand = reinterpret_cast<build_version_command *>(loadCommand);
            return makeVersionTuple(versionCommand->minos, versionCommand->sdk, osForPlatform(versionCommand->platform));
#endif
        }
        commandCursor += loadCommand->cmdsize;
    }
    Q_ASSERT_X(false, "QMacVersion", "Could not find any version load command");
    Q_UNREACHABLE();
}

QMacVersion::VersionTuple QMacVersion::applicationVersion()
{
    static VersionTuple version = []() {
        const mach_header *executableHeader = nullptr;
        for (uint32_t i = 0; i < _dyld_image_count(); ++i) {
            auto header = _dyld_get_image_header(i);
            if (header->filetype == MH_EXECUTE) {
                executableHeader = header;
                break;
            }
        }
        Q_ASSERT_X(executableHeader, "QMacVersion", "Failed to resolve Mach-O header of executable");
        return versionsForImage(executableHeader);
    }();
    return version;
}

QMacVersion::VersionTuple QMacVersion::libraryVersion()
{
    static VersionTuple version = []() {
        Dl_info qtCoreImage;
        dladdr((const void *)&QMacVersion::libraryVersion, &qtCoreImage);
        Q_ASSERT_X(qtCoreImage.dli_fbase, "QMacVersion", "Failed to resolve Mach-O header of QtCore");
        return versionsForImage(static_cast<mach_header*>(qtCoreImage.dli_fbase));
    }();
    return version;
}

// -------------------------------------------------------------------------

QT_END_NAMESPACE
