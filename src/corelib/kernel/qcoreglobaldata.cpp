/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qcoreglobaldata_p.h"
#if QT_CONFIG(textcodec)
#include "qtextcodec.h"
#endif

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QCoreGlobalData, globalInstance)

QCoreGlobalData::QCoreGlobalData()
#if QT_CONFIG(textcodec)
    : codecForLocale(nullptr)
#endif
{
}

QCoreGlobalData::~QCoreGlobalData()
{
#if QT_CONFIG(textcodec)
    codecForLocale = nullptr;
    QList<QTextCodec *> tmp = allCodecs;
    allCodecs.clear();
    codecCache.clear();
    for (QList<QTextCodec *>::const_iterator it = tmp.constBegin(); it != tmp.constEnd(); ++it)
        delete *it;
#endif
}

QCoreGlobalData *QCoreGlobalData::instance()
{
    return globalInstance();
}

QT_END_NAMESPACE
