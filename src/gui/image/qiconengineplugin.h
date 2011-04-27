/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QICONENGINEPLUGIN_H
#define QICONENGINEPLUGIN_H

#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QIconEngine;
class QIconEngineV2;

struct Q_GUI_EXPORT QIconEngineFactoryInterface : public QFactoryInterface
{
    virtual QIconEngine *create(const QString &filename) = 0;
};

#define QIconEngineFactoryInterface_iid \
    "com.trolltech.Qt.QIconEngineFactoryInterface"
Q_DECLARE_INTERFACE(QIconEngineFactoryInterface, QIconEngineFactoryInterface_iid)

class Q_GUI_EXPORT QIconEnginePlugin : public QObject, public QIconEngineFactoryInterface
{
    Q_OBJECT
    Q_INTERFACES(QIconEngineFactoryInterface:QFactoryInterface)
public:
    QIconEnginePlugin(QObject *parent = 0);
    ~QIconEnginePlugin();

    virtual QStringList keys() const = 0;
    virtual QIconEngine *create(const QString &filename) = 0;
};

// ### Qt 5: remove version 2
struct Q_GUI_EXPORT QIconEngineFactoryInterfaceV2 : public QFactoryInterface
{
    virtual QIconEngineV2 *create(const QString &filename = QString()) = 0;
};

#define QIconEngineFactoryInterfaceV2_iid \
    "com.trolltech.Qt.QIconEngineFactoryInterfaceV2"
Q_DECLARE_INTERFACE(QIconEngineFactoryInterfaceV2, QIconEngineFactoryInterfaceV2_iid)

class Q_GUI_EXPORT QIconEnginePluginV2 : public QObject, public QIconEngineFactoryInterfaceV2
{
    Q_OBJECT
    Q_INTERFACES(QIconEngineFactoryInterfaceV2:QFactoryInterface)
public:
    QIconEnginePluginV2(QObject *parent = 0);
    ~QIconEnginePluginV2();

    virtual QStringList keys() const = 0;
    virtual QIconEngineV2 *create(const QString &filename = QString()) = 0;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QICONENGINEPLUGIN_H
