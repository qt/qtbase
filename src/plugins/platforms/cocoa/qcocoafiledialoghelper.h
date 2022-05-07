/****************************************************************************
**
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

#ifndef QCOCOAFILEDIALOGHELPER_H
#define QCOCOAFILEDIALOGHELPER_H

#include <QObject>
#include <qpa/qplatformdialoghelper.h>
#include <QtCore/private/qcore_mac_p.h>

QT_DECLARE_NAMESPACED_OBJC_INTERFACE(QNSOpenSavePanelDelegate, NSObject<NSOpenSavePanelDelegate>)

QT_BEGIN_NAMESPACE

class QEventLoop;
class QFileDialog;
class QFileDialogPrivate;

class QCocoaFileDialogHelper : public QPlatformFileDialogHelper
{
public:
    QCocoaFileDialogHelper();
    virtual ~QCocoaFileDialogHelper();

    void exec() override;

    bool defaultNameFilterDisables() const override;

    bool show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent) override;
    void hide() override;
    void setDirectory(const QUrl &directory) override;
    QUrl directory() const override;
    void selectFile(const QUrl &filename) override;
    QList<QUrl> selectedFiles() const override;
    void setFilter() override;
    void selectNameFilter(const QString &filter) override;
    QString selectedNameFilter() const override;

public: // for QNSOpenSavePanelDelegate
    void panelClosed(NSInteger result);

private:
    void createNSOpenSavePanelDelegate();

    QNSOpenSavePanelDelegate *m_delegate = nil;
    QUrl m_directory;
    QEventLoop *m_eventLoop = nullptr;
};

QT_END_NAMESPACE

#endif // QCOCOAFILEDIALOGHELPER_H
