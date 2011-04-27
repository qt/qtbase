/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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

/****************************************************************************
**
** Definition of QMultiInputContextPlugin class
**
** Copyright (C) 2004 immodule for Qt Project.  All rights reserved.
**
** This file is written to contribute to Nokia Corporation and/or its subsidiary(-ies) under their own
** license. You may use this file under your Qt license. Following
** description is copied from their original file headers. Contact
** immodule-qt@freedesktop.org if any conditions of this licensing are
** not clear to you.
**
****************************************************************************/

#ifndef QMULTIINPUTCONTEXTPLUGIN_H
#define QMULTIINPUTCONTEXTPLUGIN_H

#ifndef QT_NO_IM

#include "qmultiinputcontext.h"
#include <QtGui/qinputcontextplugin.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

class QMultiInputContextPlugin : public QInputContextPlugin
{
    Q_OBJECT
public:
    QMultiInputContextPlugin();
    ~QMultiInputContextPlugin();

    QStringList keys() const;
    QInputContext *create( const QString &key );
    QStringList languages( const QString &key );
    QString displayName( const QString &key );
    QString description( const QString &key );
};

#endif // QT_NO_IM

QT_END_NAMESPACE

#endif // QMULTIINPUTCONTEXTPLUGIN_H
