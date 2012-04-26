/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCOCOAFILEDIALOGHELPER_H
#define QCOCOAFILEDIALOGHELPER_H

#include <QObject>
#include <qpa/qplatformdialoghelper.h>

QT_BEGIN_NAMESPACE

class QFileDialog;
class QFileDialogPrivate;

class QCocoaFileDialogHelper : public QPlatformFileDialogHelper
{
public:
    QCocoaFileDialogHelper();
    virtual ~QCocoaFileDialogHelper();

    void platformNativeDialogModalHelp();
    void _q_platformRunNativeAppModalPanel();

    bool defaultNameFilterDisables() const;

    void deleteNativeDialog_sys();
    bool show_sys(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent);
    void hide_sys();
    QPlatformFileDialogHelper::DialogCode dialogResultCode_sys();
    void setDirectory_sys(const QString &directory);
    QString directory_sys() const;
    void selectFile_sys(const QString &filename);
    QStringList selectedFiles_sys() const;
    void setFilter_sys();
    void selectNameFilter_sys(const QString &filter);
    QString selectedNameFilter_sys() const;

public:
    bool showCocoaFilePanel(Qt::WindowModality windowModality, QWindow *parent);
    bool hideCocoaFilePanel();

    void createNSOpenSavePanelDelegate();
    void QNSOpenSavePanelDelegate_selectionChanged(const QString &newPath);
    void QNSOpenSavePanelDelegate_panelClosed(bool accepted);
    void QNSOpenSavePanelDelegate_directoryEntered(const QString &newDir);
    void QNSOpenSavePanelDelegate_filterSelected(int menuIndex);

private:
    void *mDelegate;
};

QT_END_NAMESPACE

#endif // QCOCOAFILEDIALOGHELPER_H
