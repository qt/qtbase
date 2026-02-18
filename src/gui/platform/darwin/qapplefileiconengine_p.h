// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QAPPLEFILEICONENGINE_P_H
#define QAPPLEFILEICONENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qabstractfileiconengine_p.h>
#include <QtCore/private/qcore_mac_p.h>

Q_FORWARD_DECLARE_OBJC_CLASS(UIImage);
Q_FORWARD_DECLARE_OBJC_CLASS(NSImage);

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QAppleFileIconEngine : public QAbstractFileIconEngine
{
public:
    explicit QAppleFileIconEngine(const QFileInfo &info, QPlatformTheme::IconOptions opts);
    ~QAppleFileIconEngine() override;

    bool isNull() override;
    QList<QSize> availableSizes(QIcon::Mode = QIcon::Normal, QIcon::State = QIcon::Off) override;

protected:
    QPixmap filePixmap(const QSize &size, QIcon::Mode, QIcon::State) override;

private:
#if defined(Q_OS_MACOS)
    NSImage *m_image = nil;
#elif defined(QT_PLATFORM_UIKIT)
    UIImage *m_image = nil;
#endif

    QPixmap m_pixmap;
};


QT_END_NAMESPACE

#endif // QAPPLEFILEICONENGINE_P_H
