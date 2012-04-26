/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCOCOACOLORDIALOGHELPER_H
#define QCOCOACOLORDIALOGHELPER_H

#include <QObject>
#include <qplatformdialoghelper_qpa.h>

QT_BEGIN_NAMESPACE

class QCocoaColorDialogHelper : public QPlatformColorDialogHelper
{
public:
    QCocoaColorDialogHelper();
    virtual ~QCocoaColorDialogHelper();

    void platformNativeDialogModalHelp();
    void _q_platformRunNativeAppModalPanel();
    void deleteNativeDialog_sys();
    bool show_sys(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent);
    void hide_sys();

    DialogCode dialogResultCode_sys();
    void setCurrentColor_sys(const QColor&);
    QColor currentColor_sys() const;

public:
    bool showCocoaColorPanel(Qt::WindowModality windowModality, QWindow *parent);
    bool hideCocoaColorPanel();

    void createNSColorPanelDelegate();

private:
    void *mDelegate;
};

QT_END_NAMESPACE

#endif // QCOCOACOLORDIALOGHELPER_H
