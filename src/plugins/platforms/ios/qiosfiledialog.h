/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QIOSFILEDIALOG_H
#define QIOSFILEDIALOG_H

#include <QtCore/qeventloop.h>
#include <qpa/qplatformdialoghelper.h>

Q_FORWARD_DECLARE_OBJC_CLASS(UIViewController);

QT_BEGIN_NAMESPACE

class QIOSFileDialog : public QPlatformFileDialogHelper
{
public:
    QIOSFileDialog();
    ~QIOSFileDialog();

    void exec() Q_DECL_OVERRIDE;
    bool defaultNameFilterDisables() const Q_DECL_OVERRIDE { return false; }
    bool show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent) Q_DECL_OVERRIDE;
    void hide() Q_DECL_OVERRIDE;
    void setDirectory(const QUrl &) Q_DECL_OVERRIDE {}
    QUrl directory() const Q_DECL_OVERRIDE { return QUrl(); }
    void selectFile(const QUrl &) Q_DECL_OVERRIDE {}
    QList<QUrl> selectedFiles() const Q_DECL_OVERRIDE;
    void setFilter() Q_DECL_OVERRIDE {}
    void selectNameFilter(const QString &) Q_DECL_OVERRIDE {}
    QString selectedNameFilter() const Q_DECL_OVERRIDE { return QString(); }

    void selectedFilesChanged(QList<QUrl> selection);

private:
    QUrl m_directory;
    QList<QUrl> m_selection;
    QEventLoop m_eventLoop;
    UIViewController *m_viewController;

    bool showImagePickerDialog(QWindow *parent);
};

QT_END_NAMESPACE

#endif // QIOSFILEDIALOG_H

