// Copyright (C) 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

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
