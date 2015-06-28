/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCOCOAFILEDIALOGHELPER_H
#define QCOCOAFILEDIALOGHELPER_H

#include <QObject>
#include <qpa/qplatformdialoghelper.h>

Q_FORWARD_DECLARE_OBJC_CLASS(QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate));

QT_BEGIN_NAMESPACE

class QFileDialog;
class QFileDialogPrivate;

class QCocoaFileDialogHelper : public QPlatformFileDialogHelper
{
public:
    QCocoaFileDialogHelper();
    virtual ~QCocoaFileDialogHelper();

    void exec() Q_DECL_OVERRIDE;

    bool defaultNameFilterDisables() const Q_DECL_OVERRIDE;

    bool show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent) Q_DECL_OVERRIDE;
    void hide() Q_DECL_OVERRIDE;
    void setDirectory(const QUrl &directory) Q_DECL_OVERRIDE;
    QUrl directory() const Q_DECL_OVERRIDE;
    void selectFile(const QUrl &filename) Q_DECL_OVERRIDE;
    QList<QUrl> selectedFiles() const Q_DECL_OVERRIDE;
    void setFilter() Q_DECL_OVERRIDE;
    void selectNameFilter(const QString &filter) Q_DECL_OVERRIDE;
    QString selectedNameFilter() const Q_DECL_OVERRIDE;

public:
    bool showCocoaFilePanel(Qt::WindowModality windowModality, QWindow *parent);
    bool hideCocoaFilePanel();

    void createNSOpenSavePanelDelegate();
    void QNSOpenSavePanelDelegate_selectionChanged(const QString &newPath);
    void QNSOpenSavePanelDelegate_panelClosed(bool accepted);
    void QNSOpenSavePanelDelegate_directoryEntered(const QString &newDir);
    void QNSOpenSavePanelDelegate_filterSelected(int menuIndex);

private:
    QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *mDelegate;
    QUrl mDir;
};

QT_END_NAMESPACE

#endif // QCOCOAFILEDIALOGHELPER_H
