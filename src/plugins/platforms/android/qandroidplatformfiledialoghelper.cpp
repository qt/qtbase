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

#include "qandroidplatformfiledialoghelper.h"

#include <androidjnimain.h>
#include <private/qjni_p.h>
#include <jni.h>

QT_BEGIN_NAMESPACE

namespace QtAndroidFileDialogHelper {

#define RESULT_OK -1
#define REQUEST_CODE 1305 // Arbitrary

QAndroidPlatformFileDialogHelper::QAndroidPlatformFileDialogHelper()
    : QPlatformFileDialogHelper()
    , m_selectedFile()
{
}

bool QAndroidPlatformFileDialogHelper::handleActivityResult(jint requestCode, jint resultCode, jobject data)
{
    if (requestCode != REQUEST_CODE)
        return false;

    if (resultCode == RESULT_OK) {
        const QJNIObjectPrivate intent = QJNIObjectPrivate::fromLocalRef(data);
        const QJNIObjectPrivate uri = intent.callObjectMethod("getData", "()Landroid/net/Uri;");
        const QString uriStr = uri.callObjectMethod("toString", "()Ljava/lang/String;").toString();
        m_selectedFile = QUrl(uriStr);
        Q_EMIT fileSelected(m_selectedFile);
        Q_EMIT accept();
    } else {
        Q_EMIT reject();
    }

    return true;
}

bool QAndroidPlatformFileDialogHelper::show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent)
{
    Q_UNUSED(windowFlags)
    Q_UNUSED(windowModality)
    Q_UNUSED(parent)

    if (options()->fileMode() != QFileDialogOptions::FileMode::ExistingFile)
        return false;

    QtAndroidPrivate::registerActivityResultListener(this);

    const QJNIObjectPrivate ACTION_OPEN_DOCUMENT = QJNIObjectPrivate::getStaticObjectField("android/content/Intent", "ACTION_OPEN_DOCUMENT", "Ljava/lang/String;");
    QJNIObjectPrivate intent("android/content/Intent", "(Ljava/lang/String;)V", ACTION_OPEN_DOCUMENT.object());
    const QJNIObjectPrivate CATEGORY_OPENABLE = QJNIObjectPrivate::getStaticObjectField("android/content/Intent", "CATEGORY_OPENABLE", "Ljava/lang/String;");
    intent.callObjectMethod("addCategory", "(Ljava/lang/String;)Landroid/content/Intent;", CATEGORY_OPENABLE.object());
    intent.callObjectMethod("setType", "(Ljava/lang/String;)Landroid/content/Intent;", QJNIObjectPrivate::fromString(QStringLiteral("*/*")).object());

    const QJNIObjectPrivate activity(QtAndroid::activity());
    activity.callMethod<void>("startActivityForResult", "(Landroid/content/Intent;I)V", intent.object(), REQUEST_CODE);

    return true;
}

void QAndroidPlatformFileDialogHelper::exec()
{
    m_eventLoop.exec(QEventLoop::DialogExec);
}

void QAndroidPlatformFileDialogHelper::hide()
{
    if (m_eventLoop.isRunning())
        m_eventLoop.exit();
    QtAndroidPrivate::unregisterActivityResultListener(this);
}

QString QAndroidPlatformFileDialogHelper::selectedNameFilter() const
{
    return QString();
}

void QAndroidPlatformFileDialogHelper::selectNameFilter(const QString &filter)
{
    Q_UNUSED(filter)
}

void QAndroidPlatformFileDialogHelper::setFilter()
{
}

QList<QUrl> QAndroidPlatformFileDialogHelper::selectedFiles() const
{
    return {m_selectedFile};
}

void QAndroidPlatformFileDialogHelper::selectFile(const QUrl &file)
{
    Q_UNUSED(file)
}

QUrl QAndroidPlatformFileDialogHelper::directory() const
{
    return QUrl();
}

void QAndroidPlatformFileDialogHelper::setDirectory(const QUrl &directory)
{
    Q_UNUSED(directory)
}

bool QAndroidPlatformFileDialogHelper::defaultNameFilterDisables() const
{
    return false;
}
}

QT_END_NAMESPACE
