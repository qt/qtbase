// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHAIKUINTEGRATION_H
#define QHAIKUINTEGRATION_H

#include <qpa/qplatformintegration.h>

QT_BEGIN_NAMESPACE

class QHaikuClipboard;
class QHaikuScreen;
class QHaikuServices;

class QHaikuIntegration : public QPlatformIntegration
{
public:
    explicit QHaikuIntegration(const QStringList &paramList);
    ~QHaikuIntegration();

    bool hasCapability(QPlatformIntegration::Capability cap) const override;

    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;
    QAbstractEventDispatcher *createEventDispatcher() const override;

    QPlatformFontDatabase *fontDatabase() const override;
    QPlatformServices *services() const override;

#ifndef QT_NO_CLIPBOARD
    QPlatformClipboard *clipboard() const override;
#endif

private:
    QHaikuClipboard *m_clipboard;
    QHaikuScreen *m_screen;
    QHaikuServices *m_services;
};

QT_END_NAMESPACE

#endif
