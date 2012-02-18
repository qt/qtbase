/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef QMIMETYPE_P_H
#define QMIMETYPE_P_H

#include "qmimetype.h"

#include <QtCore/qhash.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QMimeTypePrivate : public QSharedData
{
public:
    typedef QHash<QString, QString> LocaleHash;

    QMimeTypePrivate();
    explicit QMimeTypePrivate(const QMimeType &other);

    void clear();

    bool operator==(const QMimeTypePrivate &other) const;

    void addGlobPattern(const QString &pattern);

    QString name;
    LocaleHash localeComments;
    QString genericIconName;
    QString iconName;
    QStringList globPatterns;
};

QT_END_NAMESPACE

#define QMIMETYPE_BUILDER \
    QT_BEGIN_NAMESPACE \
    static QMimeType buildQMimeType ( \
                         const QString &name, \
                         const QString &genericIconName, \
                         const QString &iconName, \
                         const QStringList &globPatterns \
                     ) \
    { \
        QMimeTypePrivate qMimeTypeData; \
        qMimeTypeData.name = name; \
        qMimeTypeData.genericIconName = genericIconName; \
        qMimeTypeData.iconName = iconName; \
        qMimeTypeData.globPatterns = globPatterns; \
        return QMimeType(qMimeTypeData); \
    } \
    QT_END_NAMESPACE

#ifdef Q_COMPILER_RVALUE_REFS
#define QMIMETYPE_BUILDER_FROM_RVALUE_REFS \
    QT_BEGIN_NAMESPACE \
    static QMimeType buildQMimeType ( \
                         QString &&name, \
                         QString &&genericIconName, \
                         QString &&iconName, \
                         QStringList &&globPatterns \
                     ) \
    { \
        QMimeTypePrivate qMimeTypeData; \
        qMimeTypeData.name = std::move(name); \
        qMimeTypeData.genericIconName = std::move(genericIconName); \
        qMimeTypeData.iconName = std::move(iconName); \
        qMimeTypeData.globPatterns = std::move(globPatterns); \
        return QMimeType(qMimeTypeData); \
    } \
    QT_END_NAMESPACE
#endif

#endif   // QMIMETYPE_P_H
