// Copyright (C) 2013 Samuel Gaist <samuel.gaist@edeltech.ch>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSSESSIONMANAGER_H
#define QWINDOWSSESSIONMANAGER_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <qpa/qplatformsessionmanager.h>

QT_BEGIN_NAMESPACE

class QWindowsSessionManager : public QPlatformSessionManager
{
public:
    explicit QWindowsSessionManager(const QString &id, const QString &key);

    bool allowsInteraction() override;
    bool allowsErrorInteraction() override;

    void blocksInteraction() { m_blockUserInput = true; }
    bool isInteractionBlocked() const { return m_blockUserInput; }

    void release() override;

    void cancel() override;
    void clearCancellation() { m_canceled = false; }
    bool wasCanceled() const { return m_canceled; }

    void setActive(bool active) { m_isActive = active; }
    bool isActive() const { return m_isActive;}

private:
    bool m_isActive = false;
    bool m_blockUserInput = false;
    bool m_canceled = false;

    Q_DISABLE_COPY_MOVE(QWindowsSessionManager)
};

QT_END_NAMESPACE

#endif // QWINDOWSSESSIONMANAGER_H
