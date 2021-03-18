/****************************************************************************
**
** Copyright (C) 2019 Klaralvdalens Datakonsult AB (KDAB)
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

#ifndef QANDROIDPLATFORMFILEDIALOGHELPER_H
#define QANDROIDPLATFORMFILEDIALOGHELPER_H

#include <jni.h>
#include <QEventLoop>
#include <qpa/qplatformdialoghelper.h>
#include <QtCore/private/qjnihelpers_p.h>
#include <private/qjni_p.h>
#include <QEventLoop>

QT_BEGIN_NAMESPACE

namespace QtAndroidFileDialogHelper {

class QAndroidPlatformFileDialogHelper: public QPlatformFileDialogHelper, public QtAndroidPrivate::ActivityResultListener
{
    Q_OBJECT

public:
    QAndroidPlatformFileDialogHelper();

    void exec() override;
    bool show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent) override;
    void hide() override;

    QString selectedNameFilter() const override { return QString(); };
    void selectNameFilter(const QString &filter) override { Q_UNUSED(filter) };
    void setFilter() override {};
    QList<QUrl> selectedFiles() const override { return m_selectedFile; };
    void selectFile(const QUrl &file) override { Q_UNUSED(file) };
    QUrl directory() const override { return QUrl(); };
    void setDirectory(const QUrl &directory) override { Q_UNUSED(directory) };
    bool defaultNameFilterDisables() const override { return false; };
    bool handleActivityResult(jint requestCode, jint resultCode, jobject data) override;

private:
    QJNIObjectPrivate getFileDialogIntent(const QString &intentType);
    void takePersistableUriPermission(const QJNIObjectPrivate &uri);
    void setIntentTitle(const QString &title);
    void setOpenableCategory();
    void setAllowMultipleSelections(bool allowMultiple);
    void setMimeTypes();

    QEventLoop m_eventLoop;
    QList<QUrl> m_selectedFile;
    QJNIObjectPrivate m_intent;
    const QJNIObjectPrivate m_activity;
};

}
QT_END_NAMESPACE

#endif // QANDROIDPLATFORMFILEDIALOGHELPER_H
