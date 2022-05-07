/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef ANDROIDCONNECTIVITYMANAGER_H
#define ANDROIDCONNECTIVITYMANAGER_H

#include <QObject>
#include <QtCore/qjniobject.h>

QT_BEGIN_NAMESPACE

class AndroidConnectivityManager : public QObject
{
    Q_OBJECT
public:
    enum class AndroidConnectivity { Connected, Unknown, Disconnected };
    Q_ENUM(AndroidConnectivity);
    static AndroidConnectivityManager *getInstance();
    ~AndroidConnectivityManager();

    AndroidConnectivity networkConnectivity();
    inline bool isValid() const { return m_connectivityManager.isValid(); }

Q_SIGNALS:
    void connectivityChanged();
    void captivePortalChanged(bool state);

private:
    friend struct AndroidConnectivityManagerInstance;
    AndroidConnectivityManager();
    bool registerNatives();
    QJniObject m_connectivityManager;

    Q_DISABLE_COPY_MOVE(AndroidConnectivityManager);
};

QT_END_NAMESPACE

#endif // ANDROIDCONNECTIVITYMANAGER_H
