// Copyright (C) 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQNXABSTRACTNAVIGATOR_H
#define QQNXABSTRACTNAVIGATOR_H

#include <QObject>

QT_BEGIN_NAMESPACE

class QUrl;

class QQnxAbstractNavigator : public QObject
{
    Q_OBJECT
public:
    explicit QQnxAbstractNavigator(QObject *parent = nullptr);
    ~QQnxAbstractNavigator();

    bool invokeUrl(const QUrl &url);

protected:
    virtual bool requestInvokeUrl(const QByteArray &encodedUrl) = 0;
};

QT_END_NAMESPACE

#endif // QQNXABSTRACTNAVIGATOR_H
