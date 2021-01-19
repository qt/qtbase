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

#ifndef QFACTORYLOADER_P_H
#define QFACTORYLOADER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qglobal.h"
#ifndef QT_NO_QOBJECT

#include "QtCore/qobject.h"
#include "QtCore/qstringlist.h"
#include "QtCore/qcborvalue.h"
#include "QtCore/qjsonobject.h"
#include "QtCore/qjsondocument.h"
#include "QtCore/qmap.h"
#include "QtCore/qendian.h"
#if QT_CONFIG(library)
#include "private/qlibrary_p.h"
#endif

QT_BEGIN_NAMESPACE

QJsonDocument qJsonFromRawLibraryMetaData(const char *raw, qsizetype size, QString *errMsg);

class QFactoryLoaderPrivate;
class Q_CORE_EXPORT QFactoryLoader : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QFactoryLoader)

public:
    explicit QFactoryLoader(const char *iid,
                   const QString &suffix = QString(),
                   Qt::CaseSensitivity = Qt::CaseSensitive);

#if QT_CONFIG(library)
    ~QFactoryLoader();

    void update();
    static void refreshAll();

#if defined(Q_OS_UNIX) && !defined (Q_OS_MAC)
    QLibraryPrivate *library(const QString &key) const;
#endif // Q_OS_UNIX && !Q_OS_MAC
#endif // QT_CONFIG(library)

    QMultiMap<int, QString> keyMap() const;
    int indexOf(const QString &needle) const;

    QList<QJsonObject> metaData() const;
    QObject *instance(int index) const;
};

template <class PluginInterface, class FactoryInterface, typename ...Args>
PluginInterface *qLoadPlugin(const QFactoryLoader *loader, const QString &key, Args &&...args)
{
    const int index = loader->indexOf(key);
    if (index != -1) {
        QObject *factoryObject = loader->instance(index);
        if (FactoryInterface *factory = qobject_cast<FactoryInterface *>(factoryObject))
            if (PluginInterface *result = factory->create(key, std::forward<Args>(args)...))
                return result;
    }
    return nullptr;
}

template <class PluginInterface, class FactoryInterface, typename Arg>
Q_DECL_DEPRECATED PluginInterface *qLoadPlugin1(const QFactoryLoader *loader, const QString &key, Arg &&arg)
{ return qLoadPlugin<PluginInterface, FactoryInterface>(loader, key, std::forward<Arg>(arg)); }

QT_END_NAMESPACE

#endif // QT_NO_QOBJECT

#endif // QFACTORYLOADER_P_H
