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

#ifndef QWINDOWSDIALOGHELPER_H
#define QWINDOWSDIALOGHELPER_H

#ifdef QT_WIDGETS_LIB

#include <QtWidgets/qplatformdialoghelper_qpa.h>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE

class QFileDialog;
class QWindowsNativeDialogBase;

namespace QWindowsDialogs
{
    enum Type { UnknownType, ColorDialog, FontDialog, FileDialog };

    Type dialogType(const QDialog *dialog);
    void eatMouseMove();
} // namespace QWindowsDialogs

class QWindowsDialogHelperBase : public QPlatformDialogHelper
{
public:
    static bool useHelper(const QDialog *dialog);
    static QPlatformDialogHelper *create(QDialog *dialog);

    virtual void platformNativeDialogModalHelp();
    virtual void _q_platformRunNativeAppModalPanel();
    virtual void deleteNativeDialog_sys();
    virtual bool setVisible_sys(bool visible);
    virtual QDialog::DialogCode dialogResultCode_sys();

    virtual bool nonNativeDialog() const = 0;
    virtual bool supportsNonModalDialog() const { return true; }

protected:
    explicit QWindowsDialogHelperBase(QDialog *dialog);
    QWindowsNativeDialogBase *nativeDialog() const;

private:
    virtual QWindowsNativeDialogBase *createNativeDialog() = 0;
    inline QWindowsNativeDialogBase *ensureNativeDialog();

    QDialog *m_dialog;
    QWindowsNativeDialogBase *m_nativeDialog;
};

QT_END_NAMESPACE

#endif // QT_WIDGETS_LIB
#endif // QWINDOWSDIALOGHELPER_H
