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

#include "qqnxservices.h"

#include "qqnxabstractnavigator.h"

QT_BEGIN_NAMESPACE

QQnxServices::QQnxServices(QQnxAbstractNavigator *navigator)
    : m_navigator(navigator)
{
}

QQnxServices::~QQnxServices()
{
}

bool QQnxServices::openUrl(const QUrl &url)
{
    return navigatorInvoke(url);
}

bool QQnxServices::openDocument(const QUrl &url)
{
    return navigatorInvoke(url);
}

bool QQnxServices::navigatorInvoke(const QUrl &url)
{
    return m_navigator->invokeUrl(url);
}

QT_END_NAMESPACE
