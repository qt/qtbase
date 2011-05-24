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

#ifndef QS60MAINDOCUMENT_H
#define QS60MAINDOCUMENT_H

#include <QtCore/qglobal.h>

#ifdef Q_OS_SYMBIAN

#ifdef Q_WS_S60
#include <AknDoc.h>
typedef CAknDocument QS60MainDocumentBase;
#else
#include <eikdoc.h>
typedef CEikDocument QS60MainDocumentBase;
#endif

class CEikApplication;

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QS60MainAppUi;

class Q_GUI_EXPORT QS60MainDocument : public QS60MainDocumentBase
{
public:

    QS60MainDocument(CEikApplication &mainApplication);
    // The virtuals are for qdoc.
    virtual ~QS60MainDocument();

public:

    virtual CEikAppUi *CreateAppUiL();

public:

    virtual CFileStore *OpenFileL(TBool aDoOpen, const TDesC &aFilename, RFs &aFs);

    virtual void OpenFileL(CFileStore *&aFileStore, RFile &aFile);
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // Q_OS_SYMBIAN

#endif // QS60MAINDOCUMENT_H
