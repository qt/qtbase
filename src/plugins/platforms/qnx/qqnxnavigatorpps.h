/***************************************************************************
**
** Copyright (C) 2012 Research In Motion
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

#ifndef QQNXNAVIGATORPPS_H
#define QQNXNAVIGATORPPS_H

#include "qqnxabstractnavigator.h"

QT_BEGIN_NAMESPACE

template <typename K, typename V> class QHash;

class QQnxNavigatorPps : public QQnxAbstractNavigator
{
    Q_OBJECT
public:
    explicit QQnxNavigatorPps(QObject *parent = 0);
    ~QQnxNavigatorPps();

protected:
    bool requestInvokeUrl(const QByteArray &encodedUrl) override;

private:
    bool openPpsConnection();

    bool sendPpsMessage(const QByteArray &message, const QByteArray &data);
    void parsePPS(const QByteArray &ppsData, QHash<QByteArray, QByteArray> &messageFields);

private:
    int m_fd;
};

QT_END_NAMESPACE

#endif // QQNXNAVIGATORPPS_H
