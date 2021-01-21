/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#ifndef QHSTSSTORE_P_H
#define QHSTSSTORE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>

QT_REQUIRE_CONFIG(settings);

#include <QtCore/qsettings.h>
#include <QtCore/qvector.h>

QT_BEGIN_NAMESPACE

class QHstsPolicy;
class QByteArray;
class QString;

class Q_AUTOTEST_EXPORT QHstsStore
{
public:
    explicit QHstsStore(const QString &dirName);
    ~QHstsStore();

    QVector<QHstsPolicy> readPolicies();
    void addToObserved(const QHstsPolicy &policy);
    void synchronize();

    bool isWritable() const;

    static QString absoluteFilePath(const QString &dirName);
private:
    void beginHstsGroups();
    bool serializePolicy(const QString &key, const QHstsPolicy &policy);
    bool deserializePolicy(const QString &key, QHstsPolicy &policy);
    void evictPolicy(const QString &key);
    void endHstsGroups();

    QVector<QHstsPolicy> observedPolicies;
    QSettings store;

    Q_DISABLE_COPY_MOVE(QHstsStore)
};

QT_END_NAMESPACE

#endif // QHSTSSTORE_P_H
