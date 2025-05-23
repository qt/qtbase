// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QCOCOACOLORDIALOGHELPER_H
#define QCOCOACOLORDIALOGHELPER_H

#include <QObject>
#include <qpa/qplatformdialoghelper.h>

QT_BEGIN_NAMESPACE

class QCocoaColorDialogHelper : public QPlatformColorDialogHelper
{
public:
    QCocoaColorDialogHelper();
    ~QCocoaColorDialogHelper();

    void exec() override;
    bool show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent) override;
    void hide() override;

    void setCurrentColor(const QColor&) override;
    QColor currentColor() const override;
};

QT_END_NAMESPACE

#endif // QCOCOACOLORDIALOGHELPER_H
