/****************************************************************************
 **
 ** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
 ** All rights reserved.
 ** Contact: Nokia Corporation (qt-info@nokia.com)
 **
 ** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QPLATFORMDIALOGHELPER_H
#define QPLATFORMDIALOGHELPER_H

#include <qglobal.h>
#include <qdialog.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QString;
class QObjectPrivate;

class Q_WIDGETS_EXPORT QPlatformDialogHelper
{
public:
    QPlatformDialogHelper();
    virtual ~QPlatformDialogHelper();

    virtual void platformNativeDialogModalHelp() = 0;
    virtual void _q_platformRunNativeAppModalPanel() = 0;

    virtual bool defaultNameFilterDisables() const = 0;

    virtual void deleteNativeDialog_sys() = 0;
    virtual bool setVisible_sys(bool visible) = 0;
    virtual QDialog::DialogCode dialogResultCode_sys() = 0;

    virtual void setDirectory_sys(const QString &directory) = 0;
    virtual QString directory_sys() const = 0;
    virtual void selectFile_sys(const QString &filename) = 0;
    virtual QStringList selectedFiles_sys() const = 0;
    virtual void setFilter_sys() = 0;
    virtual void setNameFilters_sys(const QStringList &filters) = 0;
    virtual void selectNameFilter_sys(const QString &filter) = 0;
    virtual QString selectedNameFilter_sys() const = 0;

    QObjectPrivate *d_ptr;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPLATFORMDIALOGHELPER_H
