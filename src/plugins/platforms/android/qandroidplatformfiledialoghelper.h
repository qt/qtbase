// Copyright (C) 2019 Klaralvdalens Datakonsult AB (KDAB)
// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDPLATFORMFILEDIALOGHELPER_H
#define QANDROIDPLATFORMFILEDIALOGHELPER_H

#include <jni.h>

#include <QEventLoop>
#include <QtCore/QJniObject>
#include <QtCore/private/qjnihelpers_p.h>
#include <qpa/qplatformdialoghelper.h>

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

    QString selectedNameFilter() const override { return QString(); }
    void selectNameFilter(const QString &) override {}
    void setFilter() override {}
    QList<QUrl> selectedFiles() const override { return m_selectedFile; }
    void selectFile(const QUrl &) override {}
    QUrl directory() const override { return m_directory; }
    void setDirectory(const QUrl &directory) override;
    bool defaultNameFilterDisables() const override { return false; }
    bool handleActivityResult(jint requestCode, jint resultCode, jobject data) override;

private:
    QJniObject getFileDialogIntent(const QString &intentType);
    void takePersistableUriPermission(const QJniObject &uri);
    void setInitialFileName(const QString &title);
    void setInitialDirectoryUri(const QString &directory);
    void setOpenableCategory();
    void setAllowMultipleSelections(bool allowMultiple);
    void setMimeTypes();

    QEventLoop m_eventLoop;
    QList<QUrl> m_selectedFile;
    QUrl m_directory;
    QJniObject m_intent;
    const QJniObject m_activity;
};

}
QT_END_NAMESPACE

#endif // QANDROIDPLATFORMFILEDIALOGHELPER_H
