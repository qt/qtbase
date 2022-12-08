/****************************************************************************
**
** Copyright (C) 2019 Klaralvdalens Datakonsult AB (KDAB)
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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
