/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#include "qtwindows_additional.h"
#include <QtWidgets/qplatformdialoghelper_qpa.h>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE

class QFileDialog;
class QDialog;
class QWindowsNativeDialogBase;

namespace QWindowsDialogs
{
    enum Type { UnknownType, ColorDialog, FontDialog, FileDialog };

    Type dialogType(const QDialog *dialog);
    void eatMouseMove();

    bool useHelper(const QDialog *dialog = 0);
    QPlatformDialogHelper *createHelper(QDialog *dialog = 0);
} // namespace QWindowsDialogs

template <class BaseClass>
class QWindowsDialogHelperBase : public BaseClass
{
public:

    virtual void platformNativeDialogModalHelp();
    virtual void _q_platformRunNativeAppModalPanel();
    virtual void deleteNativeDialog_sys();
    virtual bool show_sys(QPlatformDialogHelper::ShowFlags flags,
                          Qt::WindowFlags windowFlags,
                          QWindow *parent);
    virtual void hide_sys();
    virtual QVariant styleHint(QPlatformDialogHelper::StyleHint) const;

    virtual QPlatformDialogHelper::DialogCode dialogResultCode_sys();

    virtual bool supportsNonModalDialog() const { return true; }

protected:
    QWindowsDialogHelperBase();
    QWindowsNativeDialogBase *nativeDialog() const;

private:
    virtual QWindowsNativeDialogBase *createNativeDialog() = 0;
    inline QWindowsNativeDialogBase *ensureNativeDialog();

    QWindowsNativeDialogBase *m_nativeDialog;
    HWND m_ownerWindow;
};

QT_END_NAMESPACE

#endif // QT_WIDGETS_LIB
#endif // QWINDOWSDIALOGHELPER_H
