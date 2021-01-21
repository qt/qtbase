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

#ifndef QQNXABSTRACTNAVIGATOR_H
#define QQNXABSTRACTNAVIGATOR_H

#include <QObject>

QT_BEGIN_NAMESPACE

class QUrl;

class QQnxAbstractNavigator : public QObject
{
    Q_OBJECT
public:
    explicit QQnxAbstractNavigator(QObject *parent = 0);
    ~QQnxAbstractNavigator();

    bool invokeUrl(const QUrl &url);

protected:
    virtual bool requestInvokeUrl(const QByteArray &encodedUrl) = 0;
};

QT_END_NAMESPACE

#endif // QQNXABSTRACTNAVIGATOR_H
