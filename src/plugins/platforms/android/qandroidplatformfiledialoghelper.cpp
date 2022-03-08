/****************************************************************************
**
** Copyright (C) 2019 Klaralvdalens Datakonsult AB (KDAB)
** Copyright (C) 2021 The Qt Company Ltd.
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
#include <QtCore/QJniObject>

#include <QMimeDatabase>
#include <QMimeType>
#include <QRegularExpression>
#include <QUrl>

QT_BEGIN_NAMESPACE

namespace QtAndroidFileDialogHelper {

#define RESULT_OK -1
#define REQUEST_CODE 1305 // Arbitrary

const char JniIntentClass[] = "android/content/Intent";

QAndroidPlatformFileDialogHelper::QAndroidPlatformFileDialogHelper()
    : QPlatformFileDialogHelper(),
      m_activity(QtAndroid::activity())
{
}

bool QAndroidPlatformFileDialogHelper::handleActivityResult(jint requestCode, jint resultCode, jobject data)
{
    if (requestCode != REQUEST_CODE)
        return false;

    if (resultCode != RESULT_OK) {
        Q_EMIT reject();
        return true;
    }

    const QJniObject intent = QJniObject::fromLocalRef(data);

    const QJniObject uri = intent.callObjectMethod("getData", "()Landroid/net/Uri;");
    if (uri.isValid()) {
        takePersistableUriPermission(uri);
        m_selectedFile.append(QUrl(uri.toString()));
        Q_EMIT fileSelected(m_selectedFile.first());
        Q_EMIT currentChanged(m_selectedFile.first());
        Q_EMIT accept();

        return true;
    }

    const QJniObject uriClipData =
            intent.callObjectMethod("getClipData", "()Landroid/content/ClipData;");
    if (uriClipData.isValid()) {
        const int size = uriClipData.callMethod<jint>("getItemCount");
        for (int i = 0; i < size; ++i) {
            QJniObject item = uriClipData.callObjectMethod(
                    "getItemAt", "(I)Landroid/content/ClipData$Item;", i);

            QJniObject itemUri = item.callObjectMethod("getUri", "()Landroid/net/Uri;");
            takePersistableUriPermission(itemUri);
            m_selectedFile.append(itemUri.toString());
        }
        Q_EMIT filesSelected(m_selectedFile);
        Q_EMIT currentChanged(m_selectedFile.first());
        Q_EMIT accept();
    }

    return true;
}

void QAndroidPlatformFileDialogHelper::takePersistableUriPermission(const QJniObject &uri)
{
    int modeFlags = QJniObject::getStaticField<jint>(
            JniIntentClass, "FLAG_GRANT_READ_URI_PERMISSION");

    if (options()->acceptMode() == QFileDialogOptions::AcceptSave) {
        modeFlags |= QJniObject::getStaticField<jint>(
                JniIntentClass, "FLAG_GRANT_WRITE_URI_PERMISSION");
    }

    QJniObject contentResolver = m_activity.callObjectMethod(
            "getContentResolver", "()Landroid/content/ContentResolver;");
    contentResolver.callMethod<void>("takePersistableUriPermission", "(Landroid/net/Uri;I)V",
                                     uri.object(), modeFlags);
}

void QAndroidPlatformFileDialogHelper::setInitialFileName(const QString &title)
{
    const QJniObject extraTitle = QJniObject::getStaticObjectField(
            JniIntentClass, "EXTRA_TITLE", "Ljava/lang/String;");
    m_intent.callObjectMethod("putExtra",
                              "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;",
                              extraTitle.object(), QJniObject::fromString(title).object());
}

void QAndroidPlatformFileDialogHelper::setOpenableCategory()
{
    const QJniObject CATEGORY_OPENABLE = QJniObject::getStaticObjectField(
            JniIntentClass, "CATEGORY_OPENABLE", "Ljava/lang/String;");
    m_intent.callObjectMethod("addCategory", "(Ljava/lang/String;)Landroid/content/Intent;",
                              CATEGORY_OPENABLE.object());
}

void QAndroidPlatformFileDialogHelper::setAllowMultipleSelections(bool allowMultiple)
{
    const QJniObject allowMultipleSelections = QJniObject::getStaticObjectField(
            JniIntentClass, "EXTRA_ALLOW_MULTIPLE", "Ljava/lang/String;");
    m_intent.callObjectMethod("putExtra", "(Ljava/lang/String;Z)Landroid/content/Intent;",
                              allowMultipleSelections.object(), allowMultiple);
}

QStringList nameFilterExtensions(const QString nameFilters)
{
    QStringList ret;
#if QT_CONFIG(regularexpression)
    QRegularExpression re("(\\*\\.[a-z .]+)");
    QRegularExpressionMatchIterator i = re.globalMatch(nameFilters);
    while (i.hasNext())
        ret << i.next().captured(1).trimmed();
#endif // QT_CONFIG(regularexpression)
    ret.removeAll("*");
    return ret;
}

void QAndroidPlatformFileDialogHelper::setMimeTypes()
{
    QStringList mimeTypes = options()->mimeTypeFilters();
    const QStringList nameFilters = options()->nameFilters();
    const QString nameFilter = nameFilters.isEmpty() ? QString() : nameFilters.first();

    if (!nameFilter.isEmpty()) {
        QMimeDatabase db;
        for (const QString &filter : nameFilterExtensions(nameFilter))
            mimeTypes.append(db.mimeTypeForFile(filter, QMimeDatabase::MatchExtension).name());
    }

    const QString initialType = mimeTypes.size() == 1 ? mimeTypes.at(0) : QLatin1String("*/*");
    m_intent.callObjectMethod("setType", "(Ljava/lang/String;)Landroid/content/Intent;",
                              QJniObject::fromString(initialType).object());

    if (!mimeTypes.isEmpty()) {
        const QJniObject extraMimeType = QJniObject::getStaticObjectField(
                JniIntentClass, "EXTRA_MIME_TYPES", "Ljava/lang/String;");

        const QJniObject mimeTypesArray = QJniObject::callStaticObjectMethod(
                "org/qtproject/qt/android/QtNative",
                "getStringArray",
                "(Ljava/lang/String;)[Ljava/lang/String;",
                QJniObject::fromString(mimeTypes.join(",")).object());

        m_intent.callObjectMethod(
                "putExtra", "(Ljava/lang/String;[Ljava/lang/String;)Landroid/content/Intent;",
                extraMimeType.object(), mimeTypesArray.object());
    }
}

QJniObject QAndroidPlatformFileDialogHelper::getFileDialogIntent(const QString &intentType)
{
    const QJniObject ACTION_OPEN_DOCUMENT = QJniObject::getStaticObjectField(
            JniIntentClass, intentType.toLatin1(), "Ljava/lang/String;");
    return QJniObject(JniIntentClass, "(Ljava/lang/String;)V",
                             ACTION_OPEN_DOCUMENT.object());
}

bool QAndroidPlatformFileDialogHelper::show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent)
{
    Q_UNUSED(windowFlags);
    Q_UNUSED(windowModality);
    Q_UNUSED(parent);

    bool isDirDialog = false;

    m_selectedFile.clear();

    if (options()->acceptMode() == QFileDialogOptions::AcceptSave) {
        m_intent = getFileDialogIntent("ACTION_CREATE_DOCUMENT");
        const QList<QUrl> selectedFiles = options()->initiallySelectedFiles();
        if (selectedFiles.size() > 0) {
            // TODO: The initial folder to show at the start should be handled by EXTRA_INITIAL_URI
            // Take only the file name.
            setInitialFileName(selectedFiles.first().fileName());
        }
    } else if (options()->acceptMode() == QFileDialogOptions::AcceptOpen) {
        switch (options()->fileMode()) {
        case QFileDialogOptions::FileMode::DirectoryOnly:
        case QFileDialogOptions::FileMode::Directory:
            m_intent = getFileDialogIntent("ACTION_OPEN_DOCUMENT_TREE");
            isDirDialog = true;
            break;
        case QFileDialogOptions::FileMode::ExistingFiles:
            m_intent = getFileDialogIntent("ACTION_OPEN_DOCUMENT");
            setAllowMultipleSelections(true);
            break;
        case QFileDialogOptions::FileMode::AnyFile:
        case QFileDialogOptions::FileMode::ExistingFile:
            m_intent = getFileDialogIntent("ACTION_OPEN_DOCUMENT");
            break;
        }
    }

    if (!isDirDialog) {
        setOpenableCategory();
        setMimeTypes();
    }

    QtAndroidPrivate::registerActivityResultListener(this);
    m_activity.callMethod<void>("startActivityForResult", "(Landroid/content/Intent;I)V",
                              m_intent.object(), REQUEST_CODE);
    return true;
}

void QAndroidPlatformFileDialogHelper::hide()
{
    if (m_eventLoop.isRunning())
        m_eventLoop.exit();
    QtAndroidPrivate::unregisterActivityResultListener(this);
}

void QAndroidPlatformFileDialogHelper::exec()
{
    m_eventLoop.exec(QEventLoop::DialogExec);
}
}

QT_END_NAMESPACE
