// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qsystemdetection.h>

namespace QtNetworkTestHelpers
{

bool isSecureTransportBlockingTest()
{
#ifdef Q_OS_MACOS
#if QT_MACOS_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(150000, 180000)
    // Starting from macOS 15 our temporary keychain is ignored.
    // We have to use kSecImportToMemoryOnly/kCFBooleanTrue key/value
    // instead. This way we don't have to use QT_SSL_USE_TEMPORARY_KEYCHAIN anymore.
    return false;
#else
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::MacOSSequoia) {
        // We were built with SDK below 15, but a file-based keychains are not working anymore on macOS 15...
        return true;
    }
#endif
#endif // Q_OS_MACOS
    return false;
}

}


