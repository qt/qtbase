/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Symbian application wrapper of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QS60MAINAPPLICATION_H
#define QS60MAINAPPLICATION_H

#include <QtCore/qglobal.h>

#ifdef Q_OS_SYMBIAN

#ifdef Q_WS_S60
#include <aknapp.h>
typedef CAknApplication QS60MainApplicationBase;
#else
#include <eikapp.h>
typedef CEikApplication QS60MainApplicationBase;
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class Q_GUI_EXPORT QS60MainApplication : public QS60MainApplicationBase
{
public:
    QS60MainApplication();
    // The virtuals are for qdoc.
    virtual ~QS60MainApplication();

    virtual TUid AppDllUid() const;

    virtual TFileName ResourceFileName() const;

public:

    virtual void PreDocConstructL();

    virtual CDictionaryStore *OpenIniFileLC(RFs &aFs) const;

    virtual void NewAppServerL(CApaAppServer *&aAppServer);

protected:

    virtual CApaDocument *CreateDocumentL();
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // Q_OS_SYMBIAN

#endif // QS60MAINAPPLICATION_H
