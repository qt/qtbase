// Copyright (C) 2011 - 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
