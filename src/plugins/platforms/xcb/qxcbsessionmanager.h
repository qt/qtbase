// Copyright (C) 2013 Teo Mrnjavac <teo@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <qpa/qplatformsessionmanager.h>

#ifndef QT_NO_SESSIONMANAGER

QT_BEGIN_NAMESPACE

class QEventLoop;

class QXcbSessionManager : public QPlatformSessionManager
{
public:
    QXcbSessionManager(const QString &id, const QString &key);
    virtual ~QXcbSessionManager();

    void *handle() const;

    void setSessionId(const QString &id) { m_sessionId = id; }
    void setSessionKey(const QString &key) { m_sessionKey = key; }

    bool allowsInteraction() override;
    bool allowsErrorInteraction() override;
    void release() override;

    void cancel() override;

    void setManagerProperty(const QString &name, const QString &value) override;
    void setManagerProperty(const QString &name, const QStringList &value) override;

    bool isPhase2() const override;
    void requestPhase2() override;

    void exitEventLoop();

private:
    QEventLoop *m_eventLoop;
};

QT_END_NAMESPACE

#endif //QT_NO_SESSIONMANAGER
