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

#include "qqnxabstractnavigator.h"

#include <QDebug>
#include <QUrl>

#if defined(QQNXNAVIGATOR_DEBUG)
#define qNavigatorDebug qDebug
#else
#define qNavigatorDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

QQnxAbstractNavigator::QQnxAbstractNavigator(QObject *parent)
    : QObject(parent)
{
}

QQnxAbstractNavigator::~QQnxAbstractNavigator()
{
}

bool QQnxAbstractNavigator::invokeUrl(const QUrl &url)
{
    if (!url.isValid() || url.isRelative())
        return false;

    // not using QUrl::toEncoded() because for e.g. camera:// it creates camera:
    // which is not recognized by the navigator anymore
    const bool result = requestInvokeUrl(url.toString().toUtf8());

    qNavigatorDebug() << "url=" << url << "result=" << result;

    return result;
}

QT_END_NAMESPACE
