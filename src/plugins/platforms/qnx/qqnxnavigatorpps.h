// Copyright (C) 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQNXNAVIGATORPPS_H
#define QQNXNAVIGATORPPS_H

#include "qqnxabstractnavigator.h"
#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaQnxNavigator);

template <typename K, typename V> class QHash;

class QQnxNavigatorPps : public QQnxAbstractNavigator
{
    Q_OBJECT
public:
    explicit QQnxNavigatorPps(QObject *parent = nullptr);
    ~QQnxNavigatorPps();

protected:
    bool requestInvokeUrl(const QByteArray &encodedUrl) override;

private:
    bool openPpsConnection();

    bool sendPpsMessage(const QByteArray &message, const QByteArray &data);
    void parsePPS(const QByteArray &ppsData, QHash<QByteArray, QByteArray> &messageFields);

    int m_fd;
    static const char *navigatorControlPath;
    static const size_t ppsBufferSize;
};

QT_END_NAMESPACE

#endif // QQNXNAVIGATORPPS_H
