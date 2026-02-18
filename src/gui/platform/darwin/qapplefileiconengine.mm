// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qapplefileiconengine_p.h"
#include "qappleiconengine_p.h"

#if defined(Q_OS_MACOS)
# include <AppKit/AppKit.h>
#elif defined(QT_PLATFORM_UIKIT)
# include <UIKit/UIKit.h>
#endif

#include <QtCore/qurl.h>
#include <QtGui/private/qcoregraphics_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QAppleFileIconEngine::QAppleFileIconEngine(const QFileInfo &info, QPlatformTheme::IconOptions opts)
    : QAbstractFileIconEngine(info, opts)
{
#if defined(Q_OS_MACOS)
    m_image = [[NSWorkspace sharedWorkspace] iconForFile:fileInfo().canonicalFilePath().toNSString()];
#elif defined(QT_PLATFORM_UIKIT)
    const QUrl url = QUrl::fromLocalFile(fileInfo().canonicalFilePath());
    const auto controller = [UIDocumentInteractionController interactionControllerWithURL:url.toNSURL()];
    const auto allIcons = controller.icons;
    m_image = allIcons.count > 0 ? [allIcons firstObject] : nil;
#endif
    if (m_image)
        [m_image retain];
}

QAppleFileIconEngine::~QAppleFileIconEngine()
{
    if (m_image)
        [m_image release];
}

QList<QSize> QAppleFileIconEngine::availableSizes(QIcon::Mode, QIcon::State)
{
    return QAppleIconEngine::availableIconSizes();
}

bool QAppleFileIconEngine::isNull()
{
    return m_image == nil;
}

QPixmap QAppleFileIconEngine::filePixmap(const QSize &size, QIcon::Mode, QIcon::State)
{
    if (!m_image)
        return QPixmap();

    const QSize preferredSize = QSize(m_image.size.width,
                                      m_image.size.height).scaled(size, Qt::KeepAspectRatio);

    if (m_pixmap.size() == preferredSize)
        return m_pixmap;

#if defined(Q_OS_MACOS)
    m_pixmap = qt_mac_toQPixmap(m_image, preferredSize);
#elif defined(QT_PLATFORM_UIKIT)
    m_pixmap = QPixmap::fromImage(qt_mac_toQImage(m_image, preferredSize));
#endif
    return m_pixmap;
}

QT_END_NAMESPACE
