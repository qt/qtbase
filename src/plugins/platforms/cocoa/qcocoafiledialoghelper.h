/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QCOCOAFILEDIALOGHELPER_H
#define QCOCOAFILEDIALOGHELPER_H

#include <QObject>
#include <QtWidgets/qtwidgetsglobal.h>
#include <qpa/qplatformdialoghelper.h>
#include <QtCore/private/qcore_mac_p.h>

#import <AppKit/NSSavePanel.h>

QT_REQUIRE_CONFIG(filedialog);

@interface QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) : NSObject<NSOpenSavePanelDelegate>
@end

QT_NAMESPACE_ALIAS_OBJC_CLASS(QNSOpenSavePanelDelegate);

QT_BEGIN_NAMESPACE

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

public:
    bool showCocoaFilePanel(Qt::WindowModality windowModality, QWindow *parent);
    bool hideCocoaFilePanel();

    void createNSOpenSavePanelDelegate();
    void QNSOpenSavePanelDelegate_selectionChanged(const QString &newPath);
    void QNSOpenSavePanelDelegate_panelClosed(bool accepted);
    void QNSOpenSavePanelDelegate_directoryEntered(const QString &newDir);
    void QNSOpenSavePanelDelegate_filterSelected(int menuIndex);

private:
    QNSOpenSavePanelDelegate *mDelegate;
    QUrl mDir;
};

QT_END_NAMESPACE

#endif // QCOCOAFILEDIALOGHELPER_H
