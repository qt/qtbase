/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
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

#ifndef QQNXSERVICES_H
#define QQNXSERVICES_H

#include <qpa/qplatformservices.h>

QT_BEGIN_NAMESPACE

class QQnxAbstractNavigator;

class QQnxServices : public QPlatformServices
{
public:
    explicit QQnxServices(QQnxAbstractNavigator *navigator);
    ~QQnxServices();

    bool openUrl(const QUrl &url) override;
    bool openDocument(const QUrl &url) override;

private:
    bool navigatorInvoke(const QUrl &url);

private:
    QQnxAbstractNavigator *m_navigator;
};

QT_END_NAMESPACE

#endif // QQNXSERVICES_H
