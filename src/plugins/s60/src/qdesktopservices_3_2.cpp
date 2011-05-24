/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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

#include <private/qcore_symbian_p.h>
#include <qstring.h>
#include <qdir.h>

#ifdef Q_WS_S60
#include <e32base.h>                 // CBase -> Required by cdirectorylocalizer.h
#include <CDirectoryLocalizer.h>     // CDirectoryLocalizer

EXPORT_C QString localizedDirectoryName(QString& rawPath)
{
    QString ret;
    std::exception dummy;   // voodoo fix for "Undefined symbol typeinfo for std::exception" in armv5 build

    TRAPD(err,
        QT_TRYCATCH_LEAVING(
            CDirectoryLocalizer* localizer = CDirectoryLocalizer::NewL();
            CleanupStack::PushL(localizer);
            localizer->SetFullPath(qt_QString2TPtrC(QDir::toNativeSeparators(rawPath)));
            if(localizer->IsLocalized()){
                TPtrC locName(localizer->LocalizedName());
                ret = qt_TDesC2QString(locName);
            }
            CleanupStack::PopAndDestroy(localizer);
        )
    )

    if (err != KErrNone)
        ret = QString();

    return ret;
}
#else

EXPORT_C QString localizedDirectoryName(QString& /* rawPath */)
{
    qWarning("QDesktopServices::displayName() not implemented for this platform version");
    return QString();
}
#endif
