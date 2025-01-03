// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDASSETSFILEENGINEHANDLER_H
#define QANDROIDASSETSFILEENGINEHANDLER_H

#include <QtCore/private/qabstractfileengine_p.h>
#include <QCache>
#include <QMutex>
#include <QSharedPointer>

#include <android/asset_manager.h>

QT_BEGIN_NAMESPACE

class AndroidAssetsFileEngineHandler: public QAbstractFileEngineHandler
{
public:
    AndroidAssetsFileEngineHandler();
    QAbstractFileEngine *create(const QString &fileName) const override;

private:
    AAssetManager *m_assetManager;
};

QT_END_NAMESPACE

#endif // QANDROIDASSETSFILEENGINEHANDLER_H
