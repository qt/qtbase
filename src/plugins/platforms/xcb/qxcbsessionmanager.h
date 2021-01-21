/****************************************************************************
**
** Copyright (C) 2013 Teo Mrnjavac <teo@kde.org>
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QXCBSESSIONMANAGER_H
#define QXCBSESSIONMANAGER_H

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

#endif //QXCBSESSIONMANAGER_H
